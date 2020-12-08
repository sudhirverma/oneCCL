/*
 Copyright 2016-2020 Intel Corporation
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
     http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/
#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {
namespace detail {
class environment;
}

class ccl_comm_attr_impl;

namespace v1 {

struct ccl_empty_attr;

/**
 * Communicator attributes
 */
class comm_attr
        : public ccl_api_base_copyable<comm_attr, copy_on_write_access_policy, ccl_comm_attr_impl> {
public:
    using base_t =
        ccl_api_base_copyable<comm_attr, copy_on_write_access_policy, ccl_comm_attr_impl>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    comm_attr& operator=(const comm_attr& src);
    comm_attr& operator=(comm_attr&& src);
    comm_attr(comm_attr&& src);
    comm_attr(const comm_attr& src);
    comm_attr(ccl_empty_attr);
    ~comm_attr() noexcept;

    /**
     * Set specific value for selft attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <comm_attr_id attrId,
        class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <comm_attr_id attrId>
    const typename detail::ccl_api_type_attr_traits<comm_attr_id, attrId>::type& get() const;

    template <comm_attr_id attrId>
    bool is_valid() const noexcept;

private:
    friend class ccl::detail::environment;
    friend struct ccl::ccl_empty_attr;

    comm_attr(const typename detail::ccl_api_type_attr_traits<comm_attr_id,
                                                              comm_attr_id::version>::return_type&
                  version);
};
extern comm_attr default_comm_attr;

template <comm_attr_id t, class value_type>
constexpr auto attr_val(value_type v) -> detail::attr_value_triple<comm_attr_id, t, value_type> {
    return detail::attr_value_triple<comm_attr_id, t, value_type>(v);
}

} // namespace v1

using v1::comm_attr;
using v1::default_comm_attr;
using v1::attr_val;

} // namespace ccl
