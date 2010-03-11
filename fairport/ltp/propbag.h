#ifndef FAIRPORT_LTP_PROPBAG_H
#define FAIRPORT_LTP_PROPBAG_H

#include <vector>
#include <algorithm>

#include "fairport/util/primatives.h"
#include "fairport/util/errors.h"

#include "fairport/ndb/node.h"

#include "fairport/ltp/object.h"
#include "fairport/ltp/heap.h"

namespace fairport
{

typedef bth_node<prop_id, disk::prop_entry> pc_bth_node;
typedef bth_nonleaf_node<prop_id, disk::prop_entry> pc_bth_nonleaf_node;
typedef bth_leaf_node<prop_id, disk::prop_entry> pc_bth_leaf_node;

class property_bag : public const_property_object
{
public:
    explicit property_bag(const node& n);
    property_bag(const node& n, alias_tag);
    explicit property_bag(const heap& h);
    property_bag(const heap& h, alias_tag);
    property_bag(const property_bag& other);
    property_bag(const property_bag& other, alias_tag);
    property_bag(property_bag&& other) : m_pbth(std::move(other.m_pbth)) { }

    std::vector<prop_id> get_prop_list() const;
    prop_type get_prop_type(prop_id id) const
        { return (prop_type)m_pbth->lookup(id).type; }
    bool prop_exists(prop_id id) const;
    
    const node& get_node() const { return m_pbth->get_node(); }
    node& get_node() { return m_pbth->get_node(); }

private:
    property_bag& operator=(const property_bag& other); // = delete

    byte get_value_1(prop_id id) const
        { return (byte)m_pbth->lookup(id).id; }
    ushort get_value_2(prop_id id) const
        { return (ushort)m_pbth->lookup(id).id; }
    ulong get_value_4(prop_id id) const
        { return (ulong)m_pbth->lookup(id).id; }
    ulonglong get_value_8(prop_id id) const;
    std::vector<byte> get_value_variable(prop_id id) const;
    void get_prop_list_impl(std::vector<prop_id>& proplist, const pc_bth_node* pbth_node) const;

    std::unique_ptr<pc_bth_node> m_pbth;
};

} // end fairport namespace

inline fairport::property_bag::property_bag(const fairport::node& n)
{
    heap h(n, disk::heap_sig_pc);

    m_pbth = h.open_bth<prop_id, disk::prop_entry>(h.get_root_id());
}

inline fairport::property_bag::property_bag(const fairport::node& n, alias_tag)
{
    heap h(n, disk::heap_sig_pc, alias_tag());

    m_pbth = h.open_bth<prop_id, disk::prop_entry>(h.get_root_id());
}

inline fairport::property_bag::property_bag(const fairport::heap& h)
{
#ifdef FAIRPORT_VALIDATION_LEVEL_WEAK
    if(h.get_client_signature() != disk::heap_sig_pc)
        throw sig_mismatch("expected heap_sig_pc", 0, h.get_node().get_id(), h.get_client_signature(), disk::heap_sig_pc);
#endif

    heap my_heap(h);

    m_pbth = my_heap.open_bth<prop_id, disk::prop_entry>(my_heap.get_root_id());
}

inline fairport::property_bag::property_bag(const fairport::heap& h, alias_tag)
{
#ifdef FAIRPORT_VALIDATION_LEVEL_WEAK
    if(h.get_client_signature() != disk::heap_sig_pc)
        throw sig_mismatch("expected heap_sig_pc", 0, h.get_node().get_id(), h.get_client_signature(), disk::heap_sig_pc);
#endif

    heap my_heap(h, alias_tag());

    m_pbth = my_heap.open_bth<prop_id, disk::prop_entry>(my_heap.get_root_id());
}

inline fairport::property_bag::property_bag(const property_bag& other)
{
    heap h(other.m_pbth->get_node());

    m_pbth = h.open_bth<prop_id, disk::prop_entry>(h.get_root_id());
}

inline fairport::property_bag::property_bag(const property_bag& other, alias_tag)
{
    heap h(other.m_pbth->get_node(), alias_tag());

    m_pbth = h.open_bth<prop_id, disk::prop_entry>(h.get_root_id());
}

inline std::vector<fairport::prop_id> fairport::property_bag::get_prop_list() const
{
    std::vector<prop_id> proplist;

    get_prop_list_impl(proplist, m_pbth.get());

    return proplist;
}

inline void fairport::property_bag::get_prop_list_impl(std::vector<prop_id>& proplist, const pc_bth_node* pbth_node) const
{
    if(pbth_node->get_level() == 0)
    {
        // leaf
        const pc_bth_leaf_node* pleaf = static_cast<const pc_bth_leaf_node*>(pbth_node);

        for(uint i = 0; i < pleaf->num_values(); ++i)
            proplist.push_back(pleaf->get_key(i));
    }
    else
    {
        // non-leaf
        const pc_bth_nonleaf_node* pnonleaf = static_cast<const pc_bth_nonleaf_node*>(pbth_node); 
        for(uint i = 0; i < pnonleaf->num_values(); ++i)
            get_prop_list_impl(proplist, pnonleaf->get_child(i));
    }
}

inline bool fairport::property_bag::prop_exists(prop_id id) const
{
    try
    {
        (void)m_pbth->lookup(id);
    }
    catch(key_not_found<prop_id>&)
    {
        return false;
    }

    return true;
}


inline fairport::ulonglong fairport::property_bag::get_value_8(prop_id id) const
{
    std::vector<byte> buffer = get_value_variable(id);

    return *(ulonglong*)&buffer[0];
}

inline std::vector<fairport::byte> fairport::property_bag::get_value_variable(prop_id id) const
{
    heapnode_id h_id = (heapnode_id)get_value_4(id);
    std::vector<byte> buffer;

    if(h_id == 0)
        return buffer;

    if(is_subnode_id(h_id))
    {
        node sub(m_pbth->get_node().lookup(h_id));
        buffer.resize(sub.size());
        sub.read(buffer, 0);
    }
    else
    {
        buffer = m_pbth->get_heap_ptr()->read(h_id);
    }

    return buffer;
}
#endif
