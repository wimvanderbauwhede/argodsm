/**
 * @file
 * @brief This file provides the backend interface
 * @details The backend interface can be implemented independently
 *          for different interconnects or purposes.
 * @copyright Eta Scale AB. Licensed under the ArgoDSM Open Source License. See the LICENSE file for details.
 */

#ifndef argo_backend_backend_hpp
#define argo_backend_backend_hpp argo_backend_backend_hpp

#include "../data_distribution/data_distribution.hpp"
#include "../types/types.hpp"

namespace argo {
	/**
	 * @brief namespace for backend functionality
	 * @details The backend functionality in ArgoDSM is all functions that
	 *          depend inherently on the underlying communications system
	 *          used between ArgoDSM nodes. These functions need to be
	 *          implemented separately for each backend.
	 */
	namespace atomic {
		/**
		 * @brief Memory ordering for synchronization operations
		 */
		enum class memory_order {
			relaxed, ///< No synchronization
			// consume,
			acquire, ///< This operation is an acquire operation
			release, ///< This operation is a release opearation
			acq_rel, ///< Release + Acquire
			// seq_cst
		};
	}

	namespace backend {
		/**
		 * @brief initialize backend
		 * @param size the size of the global memory to initialize
		 * @warning the signature of this function may change
		 * @todo maybe this should be tied better to the concrete memory
		 *       allocated rather than a generic "initialize this size"
		 *       functionality.
		 */
		void init(std::size_t size);

		/**
		 * @brief get ArgoDSM node ID
		 * @return local ArgoDSM node ID
		 */
		node_id_t node_id();

		/**
		 * @brief get total number of ArgoDSM nodes
		 * @return total number of ArgoDSM nodes
		 */
		int number_of_nodes();

		/**
		 * @brief get base address of global memory
		 * @return the pointer to the start of the global memory
		 * @deprecated this is an implementation detail of the prototype
		 *             implementation and should not be used in new code.
		 */
		char* global_base();

		/**
		 * @brief get the total amount of global memory
		 * @return the size of the global memory
		 * @deprecated this is an implementation detail of the prototype
		 *             implementation and should not be used in new code.
		 */
		std::size_t global_size();


		/**
		 * @brief global memory space address
		 * @deprecated prototype implementation detail
		 */
		void finalize();

		/** @brief import global_ptr */
		using argo::data_distribution::global_ptr;

		/**
		 * @brief atomic swap operation on a global address
		 * @param obj pointer to the global object to swap
		 * @param desired the new value of the global object
		 * @param order Memory synchronization ordering for this operation
		 * @return the old value of the global object
		 * @note Depending on the backend, this might require that type T is trivial.
		 */
		template<typename T>
		T atomic_exchange(global_ptr<T> obj, T desired,
				atomic::memory_order order = atomic::memory_order::acq_rel);

		/**
		 * @brief global write operation
		 * @tparam T type of object to write
		 * @param obj pointer to the object to write to
		 * @param value value to store in the object
		 * @note Depending on the backend, this might require that type T is trivial.
		 */
		template<typename T>
		void store(global_ptr<T> obj, T value);

		/**
		 * @brief a simple collective barrier
		 * @param threadcount number of threads on each node that go into the barrier
		 */
		void barrier(std::size_t threadcount=1);

		/**
		 * @brief broadcast-style collective synchronization
		 * @tparam T type of synchronized object
		 * @param source the ArgoDSM node holding the authoritative copy
		 * @param ptr pointer to the object to synchronize
		 */
		template<typename T>
		void broadcast(node_id_t source, T* ptr);

		/**
		 * @brief causes a node to self-invalidate its cache,
		 *        and thus getting any updated values on subsequent accesses
		 */
		void acquire();

		/**
		 * @brief causes a node to self-downgrade its cache,
		 *        making all previous writes to be propagated to the homenode,
		 *        visible for nodes calling acquire (or other synchronization primitives)
		 */
		void release();
	} // namespace backend
} // namespace argo

#endif /* argo_backend_backend_hpp */
