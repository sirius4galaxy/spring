#include "savanna_cluster.hpp"

using namespace eosio::chain;
using namespace eosio::testing;

// auto& A=_nodes[0]; auto&  B=_nodes[1]; auto& C=_nodes[2]; auto& D=_nodes[3];

BOOST_AUTO_TEST_SUITE(savanna_disaster_recovery)

// ---------------------------------------------------------------------------------------------------
//                               Single finalizer goes down
// ---------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------
BOOST_FIXTURE_TEST_CASE(node_goes_down, savanna_cluster::cluster_t) try {
   auto& A=_nodes[0]; auto& C=_nodes[2];

   C.close();
   A.require_lib_advancing_by(4, [&]() { A.produce_blocks(4); }); // lib still advances with 3 finalizers
   C.open();
   A.push_blocks_to(C);
   A.require_lib_advancing_by(4, [&]() { A.produce_blocks(4); }); // all 4 finalizers should be back voting
   BOOST_REQUIRE(!C.is_head_missing_finalizer_votes());           // let's make sure of that
} FC_LOG_AND_RETHROW()


// ---------------------------------------------------------------------------------------------------
BOOST_FIXTURE_TEST_CASE(recover_killed_node_with_old_fsi, savanna_cluster::cluster_t) try {
   auto& A=_nodes[0]; auto& C=_nodes[2];

   auto fsi = C.save_fsi();
   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); });
   auto snapshot = C.snapshot();
   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); });
   C.close();
   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); }); // lib still advances with 3 finalizers
   C.remove_state();
   C.overwrite_fsi(fsi);
   C.open_from_snapshot(snapshot);
   A.push_blocks_to(C);
   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); }); // all 4 finalizers should be back voting
   BOOST_REQUIRE(!C.is_head_missing_finalizer_votes());           // let's make sure of that
} FC_LOG_AND_RETHROW()

// ---------------------------------------------------------------------------------------------------
BOOST_FIXTURE_TEST_CASE(recover_killed_node_with_deleted_fsi, savanna_cluster::cluster_t) try {
   auto& A=_nodes[0]; auto& C=_nodes[2];

   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); });
   auto snapshot = C.snapshot();
   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); });
   C.close();
   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); }); // lib still advances with 3 finalizers
   C.remove_state();
   C.remove_fsi();
   C.open_from_snapshot(snapshot);
   A.push_blocks_to(C);
   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); }); // all 4 finalizers should be back voting
   BOOST_REQUIRE(!C.is_head_missing_finalizer_votes());           // let's make sure of that
} FC_LOG_AND_RETHROW()

// ---------------------------------------------------------------------------------------------------
BOOST_FIXTURE_TEST_CASE(recover_killed_node_while_retaining_fsi, savanna_cluster::cluster_t) try {
   auto& A=_nodes[0]; auto& C=_nodes[2];

   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); });
   auto snapshot = C.snapshot();
   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); });
   C.close();
   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); }); // lib still advances with 3 finalizers
   C.remove_state();
   C.open_from_snapshot(snapshot);
   A.push_blocks_to(C);
   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); }); // all 4 finalizers should be back voting
   BOOST_REQUIRE(!C.is_head_missing_finalizer_votes());           // let's make sure of that
} FC_LOG_AND_RETHROW()


// ---------------------------------------------------------------------------------------------------
//                               All but one finalizers go down
// ---------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------
BOOST_FIXTURE_TEST_CASE(nodes_go_down, savanna_cluster::cluster_t) try {
   auto& A=_nodes[0]; auto& B=_nodes[1]; auto& C=_nodes[2]; auto& D=_nodes[3];
   std::array<savanna_cluster::node_t*, 3> failing_nodes { &B, &C, &D };

   for (auto& N : failing_nodes) N->close();
   A.require_lib_advancing_by(1, [&]() { A.produce_blocks(4); }); // lib stalls with 3 finalizers down
   for (auto& N : failing_nodes) N->open();
   for (auto& N : failing_nodes) A.push_blocks_to(*N);
   A.require_lib_advancing_by(7, [&]() { A.produce_blocks(4); }); // all 4 finalizers should be back voting
   for (auto& N : failing_nodes) BOOST_REQUIRE(!N->is_head_missing_finalizer_votes());
} FC_LOG_AND_RETHROW()


// ---------------------------------------------------------------------------------------------------
BOOST_FIXTURE_TEST_CASE(recover_killed_nodes_with_old_fsi, savanna_cluster::cluster_t) try {
   auto& A=_nodes[0]; auto& B=_nodes[1]; auto& C=_nodes[2]; auto& D=_nodes[3];
   std::array<savanna_cluster::node_t*, 3> failing_nodes { &B, &C, &D };

   std::vector<std::vector<uint8_t>> fsis;
   std::vector<std::string> snapshots;

   for (auto& N : failing_nodes) fsis.push_back(N->save_fsi());
   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); });
   for (auto& N : failing_nodes) snapshots.push_back(N->snapshot());
   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); });
   for (auto& N : failing_nodes) N->close();
   A.require_lib_advancing_by(1, [&]() { A.produce_blocks(2); }); // lib stalls 3 finalizers down
   size_t i = 0;
   for (auto& N : failing_nodes) {
      N->remove_state();
      N->overwrite_fsi(fsis[i]);
      N->open_from_snapshot(snapshots[i]);
      A.push_blocks_to(*N);
      ++i;
   }
   A.require_lib_advancing_by(3, [&]() { A.produce_blocks(2); }); // all 4 finalizers should be back voting
   for (auto& N : failing_nodes) BOOST_REQUIRE(!N->is_head_missing_finalizer_votes());
} FC_LOG_AND_RETHROW()

// ---------------------------------------------------------------------------------------------------
BOOST_FIXTURE_TEST_CASE(recover_killed_nodes_with_deleted_fsi, savanna_cluster::cluster_t) try {
   auto& A=_nodes[0]; auto& B=_nodes[1]; auto& C=_nodes[2]; auto& D=_nodes[3];
   std::array<savanna_cluster::node_t*, 3> failing_nodes { &B, &C, &D };

   std::vector<std::string> snapshots;

   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); });
   for (auto& N : failing_nodes) snapshots.push_back(N->snapshot());
   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); });
   for (auto& N : failing_nodes) N->close();
   A.require_lib_advancing_by(1, [&]() { A.produce_blocks(2); }); // lib stalls 3 finalizers down
   size_t i = 0;
   for (auto& N : failing_nodes) {
      N->remove_state();
      N->remove_fsi();
      N->open_from_snapshot(snapshots[i]);
      A.push_blocks_to(*N);
      ++i;
   }
   A.require_lib_advancing_by(3, [&]() { A.produce_blocks(2); }); // all 4 finalizers should be back voting
   for (auto& N : failing_nodes) BOOST_REQUIRE(!N->is_head_missing_finalizer_votes());
} FC_LOG_AND_RETHROW()

// ---------------------------------------------------------------------------------------------------
BOOST_FIXTURE_TEST_CASE(recover_killed_nodes_while_retaining_fsi, savanna_cluster::cluster_t) try {
   auto& A=_nodes[0]; auto& B=_nodes[1]; auto& C=_nodes[2]; auto& D=_nodes[3];
   std::array<savanna_cluster::node_t*, 3> failing_nodes { &B, &C, &D };

   std::vector<std::string> snapshots;

   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); });
   for (auto& N : failing_nodes) snapshots.push_back(N->snapshot());
   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); });
   for (auto& N : failing_nodes) N->close();
   A.require_lib_advancing_by(1, [&]() { A.produce_blocks(2); }); // lib stalls 3 finalizers down
   size_t i = 0;
   for (auto& N : failing_nodes) {
      N->remove_state();
      N->open_from_snapshot(snapshots[i]);
      A.push_blocks_to(*N);
      ++i;
   }
   A.require_lib_advancing_by(3, [&]() { A.produce_blocks(2); }); // all 4 finalizers should be back voting
   for (auto& N : failing_nodes) BOOST_REQUIRE(!N->is_head_missing_finalizer_votes());
} FC_LOG_AND_RETHROW()

// ---------------------------------------------------------------------------------------------------
//                      All nodes are shutdown with reversible blocks lost
// ---------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------
BOOST_FIXTURE_TEST_CASE(all_nodes_shutdown_with_reversible_blocks_lost, savanna_cluster::cluster_t) try {
   auto& A=_nodes[0]; auto& B=_nodes[1]; auto& C=_nodes[2]; auto& D=_nodes[3];
   std::array<savanna_cluster::node_t*, 4> failing_nodes { &A, &B, &C, &D };

   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); });

   // take snapshot
   // -------------
   std::string snapshot { C.snapshot() };

   // verify that all nodes have the same last irreversible block ID (lib_id) and head block ID (h_id)
   // ------------------------------------------------------------------------------------------------
   auto head_id = A.head().id();
   auto head_num = A.head().block_num();
   auto lib_id = A.lib_id;
   for (auto& N : failing_nodes) {
      BOOST_REQUIRE_EQUAL(N->head().id(), head_id);
      BOOST_REQUIRE_EQUAL(N->lib_id, lib_id);
   }

   A.require_lib_advancing_by(2, [&]() { A.produce_blocks(2); });

   // split network { A, B } and { C, D }
   // A produces two more blocks, so A and B will vote strong but finality will not advance
   // -------------------------------------------------------------------------------------
   const std::vector<size_t> partition {2, 3};
   set_partition(partition);
   A.require_lib_advancing_by(1, [&]() { A.produce_blocks(2); }); // lib stalls with network partitioned

   // remove network split
   // --------------------
   set_partition({});

   // shutdown all four nodes, delete the state and the reversible data for all nodes, but do not
   // delete the fsi or blocks log restart all four nodes from previously saved snapshot. A and B
   // finalizers will be locked on lib_id's child which was lost.
   // -----------------------------------------------------------------------------------------------
   bool remove_blocks_log = false;
   for (auto& N : failing_nodes) {
      N->close();
      N->remove_state();
      remove_blocks_log ? N->remove_reversible_data_and_blocks_log() : N->remove_reversible_data();
      N->open_from_snapshot(snapshot);
   }

   propagate_heads(); // needed only if we don't remove the blocks log

   // verify that lib does not advance and is stuck at lib_id (because validators A and B are locked on a
   // reversible block which has been lost, so they cannot vote any since the claim on the lib block
   // is just copied forward and will always be on a block with a timestamp < that the lock block in
   // the fsi)
   // ----------------------------------------------------------------------------------------------
   A.require_lib_advancing_by(0, [&]() {
      for (size_t i=0; i<4; ++i) {
         //size_t j = 0;
         for (auto& N : failing_nodes) {
            //std::cout << j++ << '\n';
            BOOST_CHECK_EQUAL(N->head().block_num(), head_num + i + (remove_blocks_log ? 0 : 1));
            if (!N->head().block()) // when restarting after removing the blocks log, `head()` has no `block()`
               continue;

            if (N == &A || N == &B) {
               // A and B are locked on a lost block so they cannot vote anymore
               BOOST_CHECK(N->is_head_missing_finalizer_votes());
            } else {
               // C and D should be able to vote
               if (i >= 2)
                  BOOST_CHECK(!N->is_head_missing_finalizer_votes());
            }
         }
         A.produce_block();
      }
   });
} FC_LOG_AND_RETHROW()




BOOST_AUTO_TEST_SUITE_END()