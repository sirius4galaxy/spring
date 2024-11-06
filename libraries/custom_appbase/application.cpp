
#include <eosio/chain/application.hpp>
#include <eosio/chain/config.hpp>

using namespace eosio::chain;

namespace appbase {


   void compatible_chain_eos() {
         config::system_account_name      = { "eosio"_n };
         config::null_account_name        = { "eosio.null"_n };
         config::producers_account_name   = { "eosio.prods"_n };
         config::eosio_auth_scope         = { "eosio.auth"_n };
         config::eosio_all_scope          = { "eosio.all"_n };
         config::eosio_any_name           = { "eosio.any"_n };
         config::eosio_code_name          = { "eosio.code"_n };
   }

   scoped_app_tester::scoped_app_tester() {
      compatible_chain_eos();
   }

   custom_scoped_app::custom_scoped_app() {
      (*this)->compatible_chain_eos_handler = compatible_chain_eos;
   }

}// namespace appbase