/**
 * @file
 * @brief MPI backend implemenation
 * @copyright Eta Scale AB. Licensed under the ArgoDSM Open Source License. See the LICENSE file for details.
 */

#include "../backend.hpp"

#include <type_traits>

#include "mpi.h"
#include "swdsm.h"

/**
 * @brief MPI communicator for node processes
 * @deprecated prototype implementation detail
 * @see swdsm.h
 * @see swdsm.cpp
 */
extern MPI_Comm workcomm;
/**
 * @todo MPI communication channel for exclusive accesses
 * @deprecated prototype implementation detail
 * @see swdsm.h
 * @see swdsm.cpp
 */
extern MPI_Win  *globalDataWindow;

/**
 * @todo should be changed to qd-locking (but need to be replaced in the other files as well)
 *       or removed when infiniband/the mpi implementations allows for multithreaded accesses to the interconnect
 * @deprecated prototype implementation detail
 */
#include <semaphore.h>
extern sem_t ibsem;

namespace argo {
	namespace backend {
		void init(std::size_t size) {
			argo_initialize(size);
		}

		node_id_t node_id() {
			return argo_get_nid();
		}

		int number_of_nodes() {
			return argo_get_nodes();
		}

		char* global_base() {
			return static_cast<char*>(argo_get_global_base());
		}

		std::size_t global_size() {
			return argo_get_global_size();
		}

		void finalize() {
			argo_finalize();
		}

		template<typename T>
		T atomic_exchange(global_ptr<T> obj, T desired, atomic::memory_order order) {
			using namespace atomic;
#if __GNUC__ >= 5 || __clang__
			static_assert(
				std::is_trivially_copy_assignable<T>::value,
				"T must be trivially copy assignable"
			);
#endif // __GNUC__ >= 5 || __clang__

			if (order == memory_order::acq_rel || order == memory_order::release)
				argo_release();

			char out_buffer[sizeof(T)];
			sem_wait(&ibsem);
			// Create a new MPI datatype with size == sizeof(T)
			MPI_Datatype t_type;
			MPI_Type_contiguous(sizeof(T), MPI_BYTE, &t_type);
			MPI_Type_commit(&t_type);
			// Perform the exchange operation
			MPI_Win_lock(MPI_LOCK_EXCLUSIVE, obj.node(), 0, globalDataWindow[0]);
			MPI_Fetch_and_op(&desired, out_buffer, t_type, obj.node(), obj.offset(), MPI_REPLACE, globalDataWindow[0]);
			MPI_Win_unlock(obj.node(), globalDataWindow[0]);
			// Cleanup
			MPI_Type_free(&t_type);
			sem_post(&ibsem);

			if (order == memory_order::acq_rel || order == memory_order::acquire)
				argo_acquire();

			// char[N] decays to char *
			return *reinterpret_cast<T*>(out_buffer);
		}

		template<typename T>
		void store(global_ptr<T> obj, T value) {
#if __GNUC__ >= 5 || __clang__
			static_assert(
				std::is_trivially_copy_assignable<T>::value,
				"T must be trivially copy assignable"
			);
#endif // __GNUC__ >= 5 || __clang__
			argo_release();
			sem_wait(&ibsem);
			// Create a new MPI datatype with size == sizeof(T)
			MPI_Datatype t_type;
			MPI_Type_contiguous(sizeof(T), MPI_BYTE, &t_type);
			MPI_Type_commit(&t_type);
			// Perform the store operation
			MPI_Win_lock(MPI_LOCK_EXCLUSIVE, obj.node(), 0, globalDataWindow[0]);
			MPI_Put(&value, 1, t_type, obj.node(), obj.offset(), 1, t_type, globalDataWindow[0]);
			MPI_Win_unlock(obj.node(), globalDataWindow[0]);
			// Cleanup
			MPI_Type_free(&t_type);
			sem_post(&ibsem);
		}

		void barrier(std::size_t tc) {
			swdsm_argo_barrier(tc);
		}

		template<typename T>
		void broadcast(node_id_t source, T* ptr) {
			sem_wait(&ibsem);
			MPI_Bcast(static_cast<void*>(ptr), sizeof(T), MPI_BYTE, source, workcomm);
			sem_post(&ibsem);
		}

		void acquire() {
			argo_acquire();
		}

		void release() {
			argo_release();
		}

#		include "../explicit_instantiations.inc.cpp"

	} // namespace backend
} // namespace argo
