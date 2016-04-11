/**
 * @file
 * @brief pseudo-backend implemenation for a single node ArgoDSM system
 * @copyright Eta Scale AB. Licensed under the Eta Scale Open Source License. See the LICENSE file for details.
 */

#include "../../data_distribution/data_distribution.hpp"
#include "../../synchronization/global_tas_lock.hpp"
#include "../../types/types.hpp"
#include "../backend.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>

/** @brief a lock for atomically executed operations */
std::mutex atomic_op_mutex;

/** @brief a lock for barrier operations */
std::mutex barrier_mutex;

/** @brief a condition variable for barrier operations */
std::condition_variable barrier_cv;

/** @brief a counter for barrier operations */
std::atomic<std::size_t> barrier_counter{0};

/** @brief a flag for in-progress barriers */
std::atomic<bool> barrier_flag{false};

/** @brief scoped locking type */
using lock_guard = std::lock_guard<std::mutex>;

/** @brief the only valid ArgoDSM node id */
const argo::node_id_t my_node_id = 0;

/** @brief the only supported amount of ArgoDSM nodes */
const std::size_t nodes = 1;

/**
 * @brief memory base pointer
 * @deprecated this is to mimic the prototype and mpi backend
 */
char* memory;

/**
 * @brief total memory size
 * @deprecated this is to mimic the prototype and mpi backend
 */
std::size_t memory_size;

namespace argo {
	namespace backend {

		void init(std::size_t size) {
			memory = new char[size];
			memory_size = size;
			using namespace data_distribution;
			naive_data_distribution<0>::set_memory_space(nodes, memory, size);
		}

		node_id_t node_id() {
			return my_node_id;
		}

		int number_of_nodes() {
			return nodes;
		}

		char* global_base() {
			return memory;
		}

		std::size_t global_size() {
			return memory_size;
		}

		void finalize() {
		}

		template<typename T>
		T atomic_exchange(global_ptr<T> obj, T desired, atomic::memory_order order) {
			(void)order; /// @todo Use the memory order
			lock_guard lock(atomic_op_mutex);
			T old = *obj;
			*obj = desired;
			return old;
		}

		template<typename T>
		void store(global_ptr<T> obj, T value) {
			*obj = value;
		}

		void barrier(std::size_t threadcount) {
			/* initially: flag = false */
			std::unique_lock<std::mutex> barrier_lock(barrier_mutex);
			barrier_counter++;
			if(barrier_counter == threadcount) {
				/* avoid progress if some thread is still stuck in the previous barrier */
				barrier_cv.wait(barrier_lock, []{ return !barrier_flag.load(); });
				/* all threads in this barrier now,
				 * flip flag to avoid progress in next barrier */
				barrier_flag = true;
				barrier_cv.notify_all();
			}

			/* wait for flag to flip to ensure all threads have reached this barrier */
			barrier_cv.wait(barrier_lock, []{ return barrier_flag.load(); });

			/* all threads synchronized here. cleanup needed. */

			barrier_counter--;
			if(barrier_counter == 0) {
				/* all threads successfully left the wait call, allow next barrier */
				barrier_flag = false;
				barrier_cv.notify_all(); // other threads + unstick next barrier's final thread
			}

			/** @bug there is a race here when the next barrier does not require
			 *       a thread stuck in this particular wait, which can then be
			 *       delayed until the end of that other barrier call
			 */
			barrier_cv.wait(barrier_lock, []{ return !barrier_flag; });
		}

		template<typename T>
		void broadcast(node_id_t source, T* ptr) {
			(void)source; // source is always node 0
			(void)ptr; // synchronization with self is a no-op
		}

#		include "../explicit_instantiations.inc.cpp"

		/** @bug should perhaps do some acq/rel on local memory */
		void acquire(){return;}
		void release(){return;}

	} // namespace backend
} // namespace argo

