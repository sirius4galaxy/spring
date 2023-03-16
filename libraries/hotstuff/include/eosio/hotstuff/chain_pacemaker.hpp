#pragma once
#include <eosio/hotstuff/base_pacemaker.hpp>
#include <eosio/chain/controller.hpp>

#include <eosio/hotstuff/qc_chain.hpp>

namespace eosio { namespace hotstuff {

	class chain_pacemaker : public base_pacemaker {

	public:

		//class-specific functions

		void init(controller* chain);

        std::mutex      _hotstuff_state_mutex;

		//base_pacemaker interface functions

       	name get_proposer();
       	name get_leader() ;
       	name get_next_leader() ;
       	std::vector<name> get_finalizers();

       	block_id_type get_current_block_id();

       	uint32_t get_quorum_threshold();

        void register_listener(name name, qc_chain& qcc);
        void unregister_listener(name name);

        void beat();

		void send_hs_proposal_msg(hs_proposal_message msg);
		void send_hs_vote_msg(hs_vote_message msg);
		void send_hs_new_block_msg(hs_new_block_message msg);
		void send_hs_new_view_msg(hs_new_view_message msg);

		void on_hs_vote_msg(hs_vote_message msg); //confirmation msg event handler
      	void on_hs_proposal_msg(hs_proposal_message msg); //consensus msg event handler
      	void on_hs_new_view_msg(hs_new_view_message msg); //new view msg event handler
      	void on_hs_new_block_msg(hs_new_block_message msg); //new block msg event handler


	private :

		chain::controller* _chain = NULL;

		qc_chain* _qc_chain = NULL;

		uint32_t _quorum_threshold = 15; //todo : calculate from schedule 

	};

}}