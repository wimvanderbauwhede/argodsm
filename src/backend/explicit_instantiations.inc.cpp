/**
 * @file
 * @brief file listing all template instances in the backend
 * @copyright Eta Scale AB. Licensed under the Eta Scale Open Source License. See the LICENSE file for details.
 */

#include "../types/types.hpp"

/**
 * @brief swap boolean flags
 * @return the old value of the flag
 */
template bool atomic_exchange(global_ptr<bool>, bool, atomic::memory_order);

/**
 * @brief swap chars
 * @return the old value of the char
 */
template char atomic_exchange(global_ptr<char>, char, atomic::memory_order);

/**
 * @brief swap integers
 * @return the old value of the integer
 */
template int atomic_exchange(global_ptr<int>, int, atomic::memory_order);

/**
 * @brief swap doubles
 * @return the old value of the double
 */
template double atomic_exchange(global_ptr<double>, double, atomic::memory_order);

/** @brief set boolean flags */
template void store(global_ptr<bool>, bool);

/** @brief set char */
template void store(global_ptr<char>, char);

/** @brief set int */
template void store(global_ptr<int>, int);

/** @brief set double */
template void store(global_ptr<double>, double);

/** @brief explicit instantiation for pointers to raw memory */
template void broadcast(node_id_t, memory_t*);
