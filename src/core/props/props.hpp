#pragma once

#include <string>
#include <vector>

#include "internal/property.hpp"
#include "mixins/all.hpp"
#include "util/serialize.hpp"

/// The property system
///
/// The property system is used for serialization, faust linking, encoder-stepping and more
/// of variables, mainly on engines.
///
/// \warning The implementation of this relies on a lot of template metaprogramming and
/// generally complicated C++. You don't need to know how it works, but I'll try to explain how
/// to use it.
namespace otto::core::props {

  template<typename T, typename Enable>
  struct default_mixins {
    using type = std::conditional_t<std::is_enum_v<T>,
                                    tag_list<change_hook, has_limits, steppable>,
                                    tag_list<change_hook>>;
  };

  template<>
  struct default_mixins<int> {
    using type = tag_list<change_hook, has_limits, steppable>;
  };

  template<>
  struct default_mixins<bool> {
    using type = tag_list<change_hook, steppable>;
  };

  template<>
  struct default_mixins<float> {
    using type = tag_list<change_hook, has_limits, steppable>;
  };

  template<>
  struct default_mixins<double> {
    using type = tag_list<change_hook, has_limits, steppable>;
  };

  // Void is for branches
  template<>
  struct default_mixins<void> {
    using type = tag_list<>;
  };

  /// A property of type `ValueType` with mixins `Tags...`
  template<typename ValueType, typename... Tags>
  using Property = PropertyImpl<ValueType, meta::_t<get_tag_list<ValueType, Tags...>>>;

  template<typename ValueType, typename... Tags>
  inline nlohmann::json serialize(const core::props::Property<ValueType, Tags...>& prop)
  {
    using ::otto::util::serialize;
    return serialize(prop.get());
  }

  template<typename ValueType, typename... Tags>
  inline void deserialize(core::props::Property<ValueType, Tags...>& prop,
                          const nlohmann::json& json)
  {
    using ::otto::util::deserialize;
    static_assert(std::is_default_constructible_v<ValueType>,
                  "A property type must be default constructible to be deserializable");
    ValueType v{};
    deserialize(v, json);
    prop.set(std::move(v));
  }
} // namespace otto::core::props
