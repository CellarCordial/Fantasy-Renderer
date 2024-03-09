#pragma once

template <typename T>
concept OperatorType = requires (T t1, T t2)
{
    t1++;
    t1--;
    t1 == t2;
    t1 != t2;
    t1 < t2;
    t1 > t2;
};