#pragma once

#include <map>

namespace sl
{

  namespace internal
  {
    template <typename K, typename V>
    struct drt_node
    {

      drt_node() : v(nullptr) {}
      struct iterator
      {
        drt_node<K, V>* ptr;
        K first;
        V second;

        auto operator->(){ return this; }
        bool operator==(const iterator& b) { return this->ptr == b.ptr; }
        bool operator!=(const iterator& b) { return this->ptr != b.ptr; }
      };

      auto end() { return iterator{nullptr, K(), V()}; }
      
      auto& find_or_create(K r, int c)
      {
        if (!r[c])
          return v;

        c++; // skip the /
        int s = c;
        while (r[c] and r[c] != '/') c++;
        K k(&r[s], c - s);
        
        return childs[k].find_or_create(r, c);
      }

      iterator find(K r, int c)
      {
        if (!r[c] and v != nullptr)
          return iterator{this, r, v};
        if (!r[c] and v == nullptr)
          return iterator{nullptr, r, v};

        c++; // skip the /
        int s = c;
        while (r[c] and r[c] != '/') c++;
        K k(&r[s], c - s);

        auto it = childs.find(k);
        if (it != childs.end())
        {
          auto it2 = it->second.find(r, c);
          if (it2 != it->second.end()) return it2;
        }

        {
          auto it2 = childs.find("*");
          if (it2 != childs.end())
            return it2->second.find(r, c);
          else
            return end();
        }
      }

      V v;
      std::map<K, drt_node> childs;
    };
  }
  
  template <typename K, typename V>
  struct dynamic_routing_table
  {

    // Find a route and return reference to a procedure.
    auto& operator[](K r)
    {
      return root.find_or_create(r, 0);
    }

    // Find a route and return an iterator.
    auto find(K r)
    {
      return root.find(r, 0);
    }

    auto end() { return root.end(); }

    internal::drt_node<K, V> root;
  };

}
