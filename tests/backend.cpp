/**
 * @file
 * @brief This file provides tests for the backends
 * @copyright Eta Scale AB. Licensed under the Eta Scale Open Source License. See the LICENSE file for details.
 */

#include <chrono>

#include "argo.hpp"
#include "data_distribution/data_distribution.hpp"

#include "gtest/gtest.h"

/** @brief Global pointer to char */
using global_char = typename argo::data_distribution::global_ptr<char>;
/** @brief Global pointer to double */
using global_double = typename argo::data_distribution::global_ptr<double>;
/** @brief Global pointer to int */
using global_int = typename argo::data_distribution::global_ptr<int>;

/** @brief ArgoDSM memory size */
constexpr std::size_t size = 1<<20; // 1MB
/** @brief Time to wait before assuming a deadlock has occured */
constexpr std::chrono::minutes deadlock_threshold{1}; // Choosen for no reason

/** @brief A random char constant */
constexpr char c_const = 'a';
/** @brief A random int constant */
constexpr int i_const = 42;
/** @brief A large random int constant */
constexpr int j_const = 2124481224;
/** @brief A random double constant */
constexpr double d_const = 1.0/3.0 * 3.14159;

/**
 * @brief Class for the gtests fixture tests. Will reset the allocators to a clean state for every test
 */
class backendTest : public testing::Test, public ::testing::WithParamInterface<int> {
	protected:
		backendTest()  {
			argo_reset();
			argo::barrier();

		}
		~backendTest() {
			argo::barrier();
		}
};

/**
 * @brief Test if atomic exchange writes the correct values
 *
 * The writes are performed by all the nodes at the same time
 */
TEST_F(backendTest, atomicXchgAll) {
	global_char _c(argo::conew_<char>(0));
	argo::backend::atomic_exchange(_c, c_const);
	ASSERT_EQ(c_const, *_c);

	global_int _i(argo::conew_<int>(0));
	argo::backend::atomic_exchange(_i, i_const);
	ASSERT_EQ(i_const, *_i);
	global_int _j(argo::conew_<int>(0));
	argo::backend::atomic_exchange(_j, j_const);
	ASSERT_EQ(j_const, *_j);

	global_double _d(argo::conew_<double>(0));
	argo::backend::atomic_exchange(_d, d_const);
	ASSERT_EQ(d_const, *_d);
}

/**
 * @brief Test if atomic exchange writes the correct values
 *
 * The writes are only performed by one node
 */
TEST_F(backendTest, atomicXchgOne) {
	global_char _c(argo::conew_<char>());
	if (argo::backend::node_id() == 0)
		argo::backend::atomic_exchange(_c, c_const);
	argo::backend::barrier();
	ASSERT_EQ(c_const, *_c);

	global_int _i(argo::conew_<int>());
	if (argo::backend::node_id() == 0)
		argo::backend::atomic_exchange(_i, i_const);
	argo::backend::barrier();
	ASSERT_EQ(i_const, *_i);
	global_int _j(argo::conew_<int>());
	if (argo::backend::node_id() == 0)
		argo::backend::atomic_exchange(_j, j_const);
	argo::backend::barrier();
	ASSERT_EQ(j_const, *_j);

	global_double _d(argo::conew_<double>());
	if (argo::backend::node_id() == 0)
		argo::backend::atomic_exchange(_d, d_const);
	argo::backend::barrier();
	ASSERT_EQ(d_const, *_d);
}

/**
 * @brief Test stores
 */
TEST_F(backendTest, storeOne) {
	global_char _c(argo::conew_<char>());
	if (argo::backend::node_id() == 0)
		argo::backend::store(_c, c_const);
	argo::backend::barrier();
	ASSERT_EQ(c_const, *_c);

	global_int _i(argo::conew_<int>());
	if (argo::backend::node_id() == 0)
		argo::backend::store(_i, i_const);
	argo::backend::barrier();
	ASSERT_EQ(i_const, *_i);
	global_int _j(argo::conew_<int>());
	if (argo::backend::node_id() == 0)
		argo::backend::store(_j, j_const);
	argo::backend::barrier();
	ASSERT_EQ(j_const, *_j);

	global_double _d(argo::conew_<double>());
	if (argo::backend::node_id() == 0)
		argo::backend::store(_d, d_const);
	argo::backend::barrier();
	ASSERT_EQ(d_const, *_d);
}

/**
 * @brief Test if atomic_exchange is indeed atomic
 *
 * A variable can be exchanged by all nodes but only one node should get the
 * initial value.
 */
TEST_F(backendTest, atomicXchgAtomicity) {
	global_int flag(argo::conew_<int>(0));
	int *rcs = argo::conew_array<int>(argo::number_of_nodes());

	// Initialize
	rcs[argo::node_id()] = 0;
	argo::barrier();

	// Do the exchange, try to get (flag == 0)
	int rc = argo::backend::atomic_exchange(flag, 1);
	rcs[argo::node_id()] = !rc;

	// Test if only one node succeeded
	argo::barrier();
	bool found = false;
	for (int i = 0; i < argo::number_of_nodes(); ++i) {
		if (rcs[i]) {
			ASSERT_FALSE(found);
			found = true;
		}
	}

	// Clean up
	argo::codelete_array(rcs);
}

/**
 * @brief Test the atomic_exchange visibility
 *
 * Go around in a circle signaling nodes using atomic_exchange and see if the
 * other shared data has also been made visible.
 */
TEST_F(backendTest, atomicXchgVisibility) {
	const int data_unset = 0xBEEF;
	const int data_set = 0x5555;
	const int flag_unset = 0xABBA;
	const int flag_set = 0x7777;

	int *shared_data = argo::conew_array<int>(argo::number_of_nodes());
	int *flag = argo::conew_array<int>(argo::number_of_nodes());
	int rc;

	int me = argo::node_id();
	int next = (me + 1) % argo::number_of_nodes();

	// Initialize
	shared_data[me] = data_unset;
	flag[me] = flag_unset;

	argo::barrier();

	// Send some data into the next node and set their flag as well
	shared_data[next] = data_set + next;
	global_int _nf(&flag[next]);
	rc = argo::backend::atomic_exchange(
		_nf, flag_set, argo::atomic::memory_order::release);
	// Nobody should have touched that but us
	ASSERT_EQ(flag_unset, rc);

	// Wait for data from the previous node
	//FIXME: There is no atomic_load yet
	int f;
	std::chrono::system_clock::time_point max_time =
		std::chrono::system_clock::now() + deadlock_threshold;
	global_int _f(&flag[me]);
	do {
		f = argo::backend::atomic_exchange(
			_f, flag_unset, argo::atomic::memory_order::acquire);
		// If we are over a minute in this loop, we have probably deadlocked.
		ASSERT_LT(std::chrono::system_clock::now(), max_time);
	} while (f == flag_unset);
	ASSERT_EQ(data_set + me, shared_data[me]);

	// Clean up
	argo::codelete_array(shared_data);
	argo::codelete_array(flag);
}

/**
 * @brief The main function that runs the tests
 * @param argc Number of command line arguments
 * @param argv Command line arguments
 * @return 0 if success
 */
int main(int argc, char **argv) {
	argo::init(size);
	::testing::InitGoogleTest(&argc, argv);
	auto res = RUN_ALL_TESTS();
	argo::finalize();
	return res;
}
