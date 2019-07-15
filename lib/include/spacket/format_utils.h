#pragma once

template<typename Wrapped>
struct Hr {
    const Wrapped& w;
};

template<typename Wrapped>
Hr<Wrapped> hr(const Wrapped& w) { return Hr<Wrapped>{w}; }
