#ifndef FAIRPORT_NDB_DATABASE_H
#define FAIRPORT_NDB_DATABASE_H

#include <fstream>
#include <memory>

#include "fairport/util/btree.h"
#include "fairport/util/errors.h"
#include "fairport/util/primatives.h"
#include "fairport/util/util.h"

#include "fairport/disk/disk.h"

#include "fairport/ndb/node.h"
#include "fairport/ndb/page.h"
#include "fairport/ndb/database_iface.h"

namespace fairport 
{ 

class node;

template<typename T> 
class database_impl;
typedef database_impl<ulonglong> large_pst;
typedef database_impl<ulong> small_pst;

shared_db_ptr open_database(const std::wstring& filename);
std::shared_ptr<small_pst> open_small_pst(const std::wstring& filename);
std::shared_ptr<large_pst> open_large_pst(const std::wstring& filename);

template<typename T>
class database_impl : public db_context
{
public:

    node lookup_node(node_id nid)
        { return node(this->shared_from_this(), lookup_node_info(nid)); }
    node_info lookup_node_info(node_id nid);
    block_info lookup_block_info(block_id bid); 
    
    // page factory functions
    std::shared_ptr<bbt_page> read_bbt_root();
    std::shared_ptr<nbt_page> read_nbt_root();
    std::shared_ptr<bbt_page> read_bbt_page(const page_info& pi);
    std::shared_ptr<nbt_page> read_nbt_page(const page_info& pi);
    std::shared_ptr<nbt_leaf_page> read_nbt_leaf_page(const page_info& pi);
    std::shared_ptr<bbt_leaf_page> read_bbt_leaf_page(const page_info& pi);
    std::shared_ptr<nbt_nonleaf_page> read_nbt_nonleaf_page(const page_info& pi);
    std::shared_ptr<bbt_nonleaf_page> read_bbt_nonleaf_page(const page_info& pi);

    std::shared_ptr<block> read_block(const shared_db_ptr& parent, block_id bid)
        { return read_block(parent, lookup_block_info(bid)); }
    std::shared_ptr<data_block> read_data_block(const shared_db_ptr& parent, block_id bid)
        { return read_data_block(parent, lookup_block_info(bid)); }
    std::shared_ptr<extended_block> read_extended_block(const shared_db_ptr& parent, block_id bid)
        { return read_extended_block(parent, lookup_block_info(bid)); }
    std::shared_ptr<external_block> read_external_block(const shared_db_ptr& parent, block_id bid)
        { return read_external_block(parent, lookup_block_info(bid)); }
    std::shared_ptr<subnode_block> read_subnode_block(const shared_db_ptr& parent, block_id bid)
        { return read_subnode_block(parent, lookup_block_info(bid)); }
    std::shared_ptr<subnode_leaf_block> read_subnode_leaf_block(const shared_db_ptr& parent, block_id bid)
        { return read_subnode_leaf_block(parent, lookup_block_info(bid)); }
    std::shared_ptr<subnode_nonleaf_block> read_subnode_nonleaf_block(const shared_db_ptr& parent, block_id bid)
        { return read_subnode_nonleaf_block(parent, lookup_block_info(bid)); }

    std::shared_ptr<block> read_block(const shared_db_ptr& parent, const block_info& bi);
    std::shared_ptr<data_block> read_data_block(const shared_db_ptr& parent, const block_info& bi);
    std::shared_ptr<extended_block> read_extended_block(const shared_db_ptr& parent, const block_info& bi);
    std::shared_ptr<external_block> read_external_block(const shared_db_ptr& parent, const block_info& bi);
    std::shared_ptr<subnode_block> read_subnode_block(const shared_db_ptr& parent, const block_info& bi);
    std::shared_ptr<subnode_leaf_block> read_subnode_leaf_block(const shared_db_ptr& parent, const block_info& bi);
    std::shared_ptr<subnode_nonleaf_block> read_subnode_nonleaf_block(const shared_db_ptr& parent, const block_info& bi);

    std::shared_ptr<external_block> create_external_block(const shared_db_ptr& parent, size_t size);
    std::shared_ptr<extended_block> create_extended_block(const shared_db_ptr& parent, std::shared_ptr<external_block>& pblock);
    std::shared_ptr<extended_block> create_extended_block(const shared_db_ptr& parent, std::shared_ptr<extended_block>& pblock);
    std::shared_ptr<extended_block> create_extended_block(const shared_db_ptr& parent, size_t size);

    block_id alloc_bid(bool is_internal);

protected:
    database_impl();
    database_impl(const std::wstring& filename);
    void validate_header();

    std::vector<byte> read_block_data(const block_info& bi);
    std::vector<byte> read_page_data(const page_info& pi);

    std::shared_ptr<nbt_leaf_page> read_nbt_leaf_page(const page_info& pi, disk::nbt_leaf_page<T>& the_page);
    std::shared_ptr<bbt_leaf_page> read_bbt_leaf_page(const page_info& pi, disk::bbt_leaf_page<T>& the_page);

    template<typename K, typename V>
    std::shared_ptr<bt_nonleaf_page<K,V>> read_bt_nonleaf_page(const page_info& pi, disk::bt_page<T, disk::bt_entry<T>>& the_page);

    std::shared_ptr<subnode_leaf_block> read_subnode_leaf_block(const shared_db_ptr& parent, const block_info& bi, disk::sub_leaf_block<T>& sub_block);
    std::shared_ptr<subnode_nonleaf_block> read_subnode_nonleaf_block(const shared_db_ptr& parent, const block_info& bi, disk::sub_nonleaf_block<T>& sub_block);

    friend shared_db_ptr open_database(const std::wstring& filename);
    friend std::shared_ptr<small_pst> open_small_pst(const std::wstring& filename);
    friend std::shared_ptr<large_pst> open_large_pst(const std::wstring& filename);

    file m_file;
    disk::header<T> m_header;
    std::shared_ptr<bbt_page> m_bbt_root;
    std::shared_ptr<nbt_page> m_nbt_root;
};

template<>
inline void database_impl<ulong>::validate_header()
{
    // the behavior of open_database depends on this throw; this can not go under FAIRPORT_VALIDATION_WEAK
    if(m_header.wVer >= disk::database_format_unicode_min)
        throw invalid_format();

#ifdef FAIRPORT_VALIDATION_LEVEL_WEAK
    ulong crc = disk::compute_crc(((byte*)&m_header) + disk::header_crc_locations<ulong>::start, disk::header_crc_locations<ulong>::length);

    if(crc != m_header.dwCRCPartial)
        throw crc_fail("header dwCRCPartial failure", 0, 0, crc, m_header.dwCRCPartial);
#endif
}

template<>
inline void database_impl<ulonglong>::validate_header()
{
    // the behavior of open_database depends on this throw; this can not go under FAIRPORT_VALIDATION_WEAK
    if(m_header.wVer < disk::database_format_unicode_min)
        throw invalid_format();

#ifdef FAIRPORT_VALIDATION_LEVEL_WEAK
    ulong crc_partial = disk::compute_crc(((byte*)&m_header) + disk::header_crc_locations<ulonglong>::partial_start, disk::header_crc_locations<ulonglong>::partial_length);
    ulong crc_full = disk::compute_crc(((byte*)&m_header) + disk::header_crc_locations<ulonglong>::full_start, disk::header_crc_locations<ulonglong>::full_length);

    if(crc_partial != m_header.dwCRCPartial)
        throw crc_fail("header dwCRCPartial failure", 0, 0, crc_partial, m_header.dwCRCPartial);

    if(crc_full != m_header.dwCRCFull)
        throw crc_fail("header dwCRCFull failure", 0, 0, crc_full, m_header.dwCRCFull);
#endif
}

} // end namespace

inline fairport::shared_db_ptr fairport::open_database(const std::wstring& filename)
{
    try 
    {
        shared_db_ptr db = open_small_pst(filename);
        return db;
    }
    catch(invalid_format&)
    {
        // well, that didn't work
    }

    shared_db_ptr db = open_large_pst(filename);
    return db;
}

inline std::shared_ptr<fairport::small_pst> fairport::open_small_pst(const std::wstring& filename)
{
    std::shared_ptr<small_pst> db(new small_pst(filename));
    return db;
}

inline std::shared_ptr<fairport::large_pst> fairport::open_large_pst(const std::wstring& filename)
{
    std::shared_ptr<large_pst> db(new large_pst(filename));
    return db;
}

template<typename T>
inline std::vector<fairport::byte> fairport::database_impl<T>::read_block_data(const block_info& bi)
{
    size_t aligned_size = disk::align_disk<T>(bi.size);

#ifdef FAIRPORT_VALIDATION_LEVEL_WEAK
    if(aligned_size > disk::max_block_disk_size)
        throw unexpected_block("nonsensical block size");

    if(bi.address + aligned_size > m_header.root_info.ibFileEof)
        throw unexpected_block("nonsensical block location; past eof");
#endif

    std::vector<byte> buffer(aligned_size);
    disk::block_trailer<T>* bt = (disk::block_trailer<T>*)(&buffer[0] + aligned_size - sizeof(disk::block_trailer<T>));

    m_file.read(buffer, bi.address);    

#ifdef FAIRPORT_VALIDATION_LEVEL_WEAK
    if(bt->bid != bi.id)
        throw unexpected_block("wrong block id");

    if(bt->cb != bi.size)
        throw unexpected_block("wrong block size");

    if(bt->signature != disk::compute_signature(bi.id, bi.address))
        throw sig_mismatch("block sig mismatch", bi.address, bi.id, disk::compute_signature(bi.id, bi.address), bt->signature);
#endif

#ifdef FAIRPORT_VALIDATION_LEVEL_FULL
    ulong crc = disk::compute_crc(&buffer[0], bi.size);
    if(crc != bt->crc)
        throw crc_fail("block crc failure", bi.address, bi.id, crc, bt->crc);
#endif

    return buffer;
}

template<typename T>
std::vector<fairport::byte> fairport::database_impl<T>::read_page_data(const page_info& pi)
{
#ifdef FAIRPORT_VALIDATION_LEVEL_WEAK
    if(pi.address + disk::page_size > m_header.root_info.ibFileEof)
        throw unexpected_page("nonsensical page location; past eof");

    if(((pi.address - disk::first_amap_page_location) % disk::page_size) != 0)
        throw unexpected_page("nonsensical page location; not sector aligned");
#endif

    std::vector<byte> buffer(disk::page_size);
    disk::page<T>* ppage = (disk::page<T>*)&buffer[0];
    
    m_file.read(buffer, pi.address);

#ifdef FAIRPORT_VALIDATION_LEVEL_FULL
    ulong crc = disk::compute_crc(&buffer[0], disk::page<T>::page_data_size);
    if(crc != ppage->trailer.crc)
        throw crc_fail("page crc failure", pi.address, pi.id, crc, ppage->trailer.crc);
#endif

#ifdef FAIRPORT_VALIDATION_LEVEL_WEAK
    if(ppage->trailer.bid != pi.id)
        throw unexpected_page("wrong page id");

    if(ppage->trailer.page_type != ppage->trailer.page_type_repeat)
        throw database_corrupt("ptype != ptype repeat?");

    if(ppage->trailer.signature != disk::compute_signature(pi.id, pi.address))
        throw sig_mismatch("page sig mismatch", pi.address, pi.id, disk::compute_signature(pi.id, pi.address), ppage->trailer.signature);
#endif

    return buffer;
}


template<typename T>
inline std::shared_ptr<fairport::bbt_page> fairport::database_impl<T>::read_bbt_root()
{ 
    if(!m_bbt_root)
    {
        page_info pi = { m_header.root_info.brefBBT.bid, m_header.root_info.brefBBT.ib };
        m_bbt_root = read_bbt_page(pi); 
    }

    return m_bbt_root;
}

template<typename T>
inline std::shared_ptr<fairport::nbt_page> fairport::database_impl<T>::read_nbt_root()
{ 
    if(!m_nbt_root)
    {
        page_info pi = { m_header.root_info.brefNBT.bid, m_header.root_info.brefNBT.ib };
        m_nbt_root = read_nbt_page(pi);
    }

    return m_nbt_root;
}

template<typename T>
inline fairport::database_impl<T>::database_impl(const std::wstring& filename)
: m_file(filename)
{
    std::vector<byte> buffer(sizeof(m_header));
    m_file.read(buffer, 0);
    memcpy(&m_header, &buffer[0], sizeof(m_header));

    validate_header();
}

template<typename T>
inline std::shared_ptr<fairport::nbt_leaf_page> fairport::database_impl<T>::read_nbt_leaf_page(const page_info& pi)
{
    std::vector<byte> buffer = read_page_data(pi);
    disk::page<T>* ppage = (disk::page<T>*)&buffer[0];
    
    if(ppage->trailer.page_type == disk::page_type_nbt)
    {
        disk::nbt_leaf_page<T>* leaf_page = (disk::nbt_leaf_page<T>*)ppage;

        if(leaf_page->level == 0)
            return read_nbt_leaf_page(pi, *leaf_page);
    }

    throw unexpected_page("page_type != page_type_nbt");
}

template<typename T>
inline std::shared_ptr<fairport::nbt_leaf_page> fairport::database_impl<T>::read_nbt_leaf_page(const page_info& pi, disk::nbt_leaf_page<T>& the_page)
{
    node_info ni;
    std::vector<std::pair<node_id, node_info>> nodes;

    for(int i = 0; i < the_page.num_entries; ++i)
    {
        ni.id = static_cast<node_id>(the_page.entries[i].nid);
        ni.data_bid = the_page.entries[i].data;
        ni.sub_bid = the_page.entries[i].sub;
        ni.parent_id = the_page.entries[i].parent_nid;

        nodes.push_back(std::make_pair(ni.id, ni));
    }

    return std::shared_ptr<nbt_leaf_page>(new nbt_leaf_page(shared_from_this(), pi, std::move(nodes)));
}

template<typename T>
inline std::shared_ptr<fairport::bbt_leaf_page> fairport::database_impl<T>::read_bbt_leaf_page(const page_info& pi)
{
    std::vector<byte> buffer = read_page_data(pi);
    disk::page<T>* ppage = (disk::page<T>*)&buffer[0];
    
    if(ppage->trailer.page_type == disk::page_type_bbt)
    {
        disk::bbt_leaf_page<T>* leaf_page = (disk::bbt_leaf_page<T>*)ppage;

        if(leaf_page->level == 0)
            return read_bbt_leaf_page(pi, *leaf_page);
    }

    throw unexpected_page("page_type != page_type_bbt");
}

template<typename T>
inline std::shared_ptr<fairport::bbt_leaf_page> fairport::database_impl<T>::read_bbt_leaf_page(const page_info& pi, disk::bbt_leaf_page<T>& the_page)
{
    block_info bi;
    std::vector<std::pair<block_id, block_info>> blocks;
    
    for(int i = 0; i < the_page.num_entries; ++i)
    {
        bi.id = the_page.entries[i].ref.bid;
        bi.address = the_page.entries[i].ref.ib;
        bi.size = the_page.entries[i].size;
        bi.ref_count = the_page.entries[i].ref_count;

        blocks.push_back(std::make_pair(bi.id, bi));
    }

    return std::shared_ptr<bbt_leaf_page>(new bbt_leaf_page(shared_from_this(), pi, std::move(blocks)));
}

template<typename T>
inline std::shared_ptr<fairport::nbt_nonleaf_page> fairport::database_impl<T>::read_nbt_nonleaf_page(const page_info& pi)
{
    std::vector<byte> buffer = read_page_data(pi);
    disk::page<T>* ppage = (disk::page<T>*)&buffer[0];
    
    if(ppage->trailer.page_type == disk::page_type_nbt)
    {
        disk::nbt_nonleaf_page<T>* nonleaf_page = (disk::nbt_nonleaf_page<T>*)ppage;

        if(nonleaf_page->level > 0)
            return read_bt_nonleaf_page<node_id, node_info>(pi, *nonleaf_page);
    }

    throw unexpected_page("page_type != page_type_nbt");
}

template<typename T>
template<typename K, typename V>
inline std::shared_ptr<fairport::bt_nonleaf_page<K,V>> fairport::database_impl<T>::read_bt_nonleaf_page(const page_info& pi, fairport::disk::bt_page<T, disk::bt_entry<T>>& the_page)
{
    std::vector<std::pair<K, page_info>> nodes;
    
    for(int i = 0; i < the_page.num_entries; ++i)
    {
        page_info subpi = { the_page.entries[i].ref.bid, the_page.entries[i].ref.ib };
        nodes.push_back(std::make_pair(static_cast<K>(the_page.entries[i].key), subpi));
    }

    return std::shared_ptr<bt_nonleaf_page<K,V>>(new bt_nonleaf_page<K,V>(shared_from_this(), pi, the_page.level, std::move(nodes)));
}

template<typename T>
inline std::shared_ptr<fairport::bbt_nonleaf_page> fairport::database_impl<T>::read_bbt_nonleaf_page(const page_info& pi)
{
    std::vector<byte> buffer = read_page_data(pi);
    disk::page<T>* ppage = (disk::page<T>*)&buffer[0];
    
    if(ppage->trailer.page_type == disk::page_type_bbt)
    {
        disk::bbt_nonleaf_page<T>* nonleaf_page = (disk::bbt_nonleaf_page<T>*)ppage;

        if(nonleaf_page->level > 0)
            return read_bt_nonleaf_page<block_id, block_info>(pi, *nonleaf_page);
    }

    throw unexpected_page("page_type != page_type_bbt");
}

template<typename T>
inline std::shared_ptr<fairport::bbt_page> fairport::database_impl<T>::read_bbt_page(const page_info& pi)
{
    std::vector<byte> buffer = read_page_data(pi);
    disk::page<T>* ppage = (disk::page<T>*)&buffer[0];

    if(ppage->trailer.page_type == disk::page_type_bbt)
    {
        disk::bbt_leaf_page<T>* leaf = (disk::bbt_leaf_page<T>*)ppage;
        if(leaf->level == 0)
        {
            // it really is a leaf!
            return read_bbt_leaf_page(pi, *leaf);
        }
        else
        {
            disk::bbt_nonleaf_page<T>* nonleaf = (disk::bbt_nonleaf_page<T>*)ppage;
            return read_bt_nonleaf_page<block_id, block_info>(pi, *nonleaf);
        }
    }
    else
    {
        throw unexpected_page("page_type != page_type_bbt");
    }  
}
        
template<typename T>
inline std::shared_ptr<fairport::nbt_page> fairport::database_impl<T>::read_nbt_page(const page_info& pi)
{
    std::vector<byte> buffer = read_page_data(pi);
    disk::page<T>* ppage = (disk::page<T>*)&buffer[0];

    if(ppage->trailer.page_type == disk::page_type_nbt)
    {
        disk::nbt_leaf_page<T>* leaf = (disk::nbt_leaf_page<T>*)ppage;
        if(leaf->level == 0)
        {
            // it really is a leaf!
            return read_nbt_leaf_page(pi, *leaf);
        }
        else
        {
            disk::nbt_nonleaf_page<T>* nonleaf = (disk::nbt_nonleaf_page<T>*)ppage;
            return read_bt_nonleaf_page<node_id, node_info>(pi, *nonleaf);
        }
    }
    else
    {
        throw unexpected_page("page_type != page_type_nbt");
    }  
}

template<typename T>
inline fairport::node_info fairport::database_impl<T>::lookup_node_info(node_id nid)
{
    return read_nbt_root()->lookup(nid); 
}

template<typename T>
inline fairport::block_info fairport::database_impl<T>::lookup_block_info(block_id bid)
{
    if(bid == 0)
    {
        block_info bi;
        bi.id = bi.address = bi.size = bi.ref_count = 0;
        return bi;
    }
    else
    {
        return read_bbt_root()->lookup(bid);
    }
}

template<typename T>
inline std::shared_ptr<fairport::block> fairport::database_impl<T>::read_block(const shared_db_ptr& parent, const block_info& bi)
{
    std::shared_ptr<block> pblock;

    try
    {
        pblock = read_data_block(parent, bi);
    }
    catch(unexpected_block&)
    {
        pblock = read_subnode_block(parent, bi);
    }

    return pblock;
}

template<typename T>
inline std::shared_ptr<fairport::data_block> fairport::database_impl<T>::read_data_block(const shared_db_ptr& parent, const block_info& bi)
{
    if(disk::bid_is_external(bi.id))
        return read_external_block(parent, bi);

    std::vector<byte> buffer(sizeof(disk::extended_block<T>));
    disk::extended_block<T>* peblock = (disk::extended_block<T>*)&buffer[0];
    m_file.read(buffer, bi.address);

    // the behavior of read_block depends on this throw; this can not go under FAIRPORT_VALIDATION_WEAK
    if(peblock->block_type != disk::block_type_extended)
        throw unexpected_block("extended block expected");

    return read_extended_block(parent, bi);
}

template<typename T>
inline std::shared_ptr<fairport::extended_block> fairport::database_impl<T>::read_extended_block(const shared_db_ptr& parent, const block_info& bi)
{
    if(!disk::bid_is_internal(bi.id))
        throw unexpected_block("internal bid expected");

    std::vector<byte> buffer = read_block_data(bi);
    disk::extended_block<T>* peblock = (disk::extended_block<T>*)&buffer[0];
    std::vector<block_id> child_blocks;

    for(int i = 0; i < peblock->count; ++i)
        child_blocks.push_back(peblock->bid[i]);

#ifdef __GNUC__
    // GCC gave link errors on extended_block<T> and external_block<T> max_size
    // with the below alernative
    uint sub_size = 0;
    if(peblock->level == 1)
        sub_size = disk::external_block<T>::max_size;
    else
        sub_size = disk::extended_block<T>::max_size;
#else
    uint sub_size = (peblock->level == 1 ? disk::external_block<T>::max_size : disk::extended_block<T>::max_size);
#endif
    uint sub_page_count = peblock->level == 1 ? 1 : disk::extended_block<T>::max_count;

    return std::shared_ptr<extended_block>(new extended_block(parent, bi, peblock->level, peblock->total_size, sub_size, disk::extended_block<T>::max_count, sub_page_count, std::move(child_blocks)));
}

template<typename T>
inline std::shared_ptr<fairport::external_block> fairport::database_impl<T>::create_external_block(const shared_db_ptr& parent, size_t size)
{
    return std::shared_ptr<external_block>(new external_block(parent, disk::external_block<T>::max_size, size));
}

template<typename T>
inline std::shared_ptr<fairport::extended_block> fairport::database_impl<T>::create_extended_block(const shared_db_ptr& parent, std::shared_ptr<external_block>& pchild_block)
{
    std::vector<std::shared_ptr<data_block>> child_blocks;
    child_blocks.push_back(pchild_block);

    return std::shared_ptr<extended_block>(new extended_block(parent, 1, pchild_block->get_total_size(), disk::external_block<T>::max_size, disk::extended_block<T>::max_count, 1, std::move(child_blocks)));
}

template<typename T>
inline std::shared_ptr<fairport::extended_block> fairport::database_impl<T>::create_extended_block(const shared_db_ptr& parent, std::shared_ptr<extended_block>& pchild_block)
{
    std::vector<std::shared_ptr<data_block>> child_blocks;
    child_blocks.push_back(pchild_block);

    assert(pchild_block->get_level() == 1);

    return std::shared_ptr<extended_block>(new extended_block(parent, 2, pchild_block->get_total_size(), disk::extended_block<T>::max_size, disk::extended_block<T>::max_count, disk::extended_block<T>::max_count, std::move(child_blocks)));
}

template<typename T>
inline std::shared_ptr<fairport::extended_block> fairport::database_impl<T>::create_extended_block(const shared_db_ptr& parent, size_t size)
{
    ushort level = size > disk::extended_block<T>::max_size ? 2 : 1;
#ifdef __GNUC__
    // More strange link errors
    size_t child_max_size;
    if(level == 1)
        child_max_size = disk::external_block<T>::max_size;
    else
        child_max_size = disk::extended_block<T>::max_size;
#else
    size_t child_max_size = level == 1 ? disk::external_block<T>::max_size : disk::extended_block<T>::max_size;
#endif
    ulong child_max_blocks = level == 1 ? 1 : disk::extended_block<T>::max_count;

    return std::shared_ptr<extended_block>(new extended_block(parent, level, size, child_max_size, disk::extended_block<T>::max_count, child_max_blocks));
}

template<typename T>
inline std::shared_ptr<fairport::external_block> fairport::database_impl<T>::read_external_block(const shared_db_ptr& parent, const block_info& bi)
{
    if(bi.id == 0)
    {
        return std::shared_ptr<external_block>(new external_block(parent, bi, disk::external_block<T>::max_size,  std::vector<byte>()));
    }

    if(!disk::bid_is_external(bi.id))
        throw unexpected_block("External BID expected");

    std::vector<byte> buffer = read_block_data(bi);

    if(m_header.bCryptMethod == disk::crypt_method_permute)
    {
        disk::permute(&buffer[0], bi.size, false);
    }
    else if(m_header.bCryptMethod == disk::crypt_method_cyclic)
    {
        disk::cyclic(&buffer[0], bi.size, (ulong)bi.id);
    }

    return std::shared_ptr<external_block>(new external_block(parent, bi, disk::external_block<T>::max_size, std::move(buffer)));
}

template<typename T>
inline std::shared_ptr<fairport::subnode_block> fairport::database_impl<T>::read_subnode_block(const shared_db_ptr& parent, const block_info& bi)
{
    if(bi.id == 0)
    {
        return std::shared_ptr<subnode_block>(new subnode_leaf_block(parent, bi, std::vector<std::pair<node_id, subnode_info>>()));
    }
    
    std::vector<byte> buffer = read_block_data(bi);
    disk::sub_leaf_block<T>* psub = (disk::sub_leaf_block<T>*)&buffer[0];
    std::shared_ptr<subnode_block> sub_block;

    if(psub->level == 0)
    {
        sub_block = read_subnode_leaf_block(parent, bi, *psub);
    }
    else
    {
        sub_block = read_subnode_nonleaf_block(parent, bi, *(disk::sub_nonleaf_block<T>*)&buffer[0]);
    }

    return sub_block;
}

template<typename T>
inline std::shared_ptr<fairport::subnode_leaf_block> fairport::database_impl<T>::read_subnode_leaf_block(const shared_db_ptr& parent, const block_info& bi)
{
    std::vector<byte> buffer = read_block_data(bi);
    disk::sub_leaf_block<T>* psub = (disk::sub_leaf_block<T>*)&buffer[0];
    std::shared_ptr<subnode_leaf_block> sub_block;

    if(psub->level == 0)
    {
        sub_block = read_subnode_leaf_block(parent, bi, *psub);
    }
    else
    {
        throw unexpected_block("psub->level != 0");
    }

    return sub_block; 
}

template<typename T>
inline std::shared_ptr<fairport::subnode_nonleaf_block> fairport::database_impl<T>::read_subnode_nonleaf_block(const shared_db_ptr& parent, const block_info& bi)
{
    std::vector<byte> buffer = read_block_data(bi);
    disk::sub_nonleaf_block<T>* psub = (disk::sub_nonleaf_block<T>*)&buffer[0];
    std::shared_ptr<subnode_nonleaf_block> sub_block;

    if(psub->level != 0)
    {
        sub_block = read_subnode_nonleaf_block(parent, bi, *psub);
    }
    else
    {
        throw unexpected_block("psub->level == 1");
    }

    return sub_block;
}

template<typename T>
inline std::shared_ptr<fairport::subnode_leaf_block> fairport::database_impl<T>::read_subnode_leaf_block(const shared_db_ptr& parent, const block_info& bi, disk::sub_leaf_block<T>& sub_block)
{
    subnode_info ni;
    std::vector<std::pair<node_id, subnode_info>> subnodes;

    for(int i = 0; i < sub_block.count; ++i)
    {
        ni.id = sub_block.entry[i].nid;
        ni.data_bid = sub_block.entry[i].data;
        ni.sub_bid = sub_block.entry[i].sub;

        subnodes.push_back(std::make_pair(sub_block.entry[i].nid, ni));
    }

    return std::shared_ptr<subnode_leaf_block>(new subnode_leaf_block(parent, bi, std::move(subnodes)));
}

template<typename T>
inline std::shared_ptr<fairport::subnode_nonleaf_block> fairport::database_impl<T>::read_subnode_nonleaf_block(const shared_db_ptr& parent, const block_info& bi, disk::sub_nonleaf_block<T>& sub_block)
{
    std::vector<std::pair<node_id, block_id>> subnodes;

    for(int i = 0; i < sub_block.count; ++i)
    {
        subnodes.push_back(std::make_pair(sub_block.entry[i].nid_key, sub_block.entry[i].sub_block_bid));
    }

    return std::shared_ptr<subnode_nonleaf_block>(new subnode_nonleaf_block(parent, bi, std::move(subnodes)));
}

template<typename T>
inline fairport::block_id fairport::database_impl<T>::alloc_bid(bool is_internal)
{
#ifdef __GNUC__
    typename disk::header<T>::block_id_disk disk_id;
    memcpy(&disk_id, m_header.bidNextB, sizeof(disk_id));

    block_id next_bid = disk_id;

    disk_id += disk::block_id_increment;
    memcpy(m_header.bidNextB, &disk_id, sizeof(disk_id));
#else
    block_id next_bid = m_header.bidNextB;
    m_header.bidNextB += disk::block_id_increment;
#endif


    if(is_internal)
        next_bid &= disk::block_id_internal_bit;

    return next_bid;
}
#endif
