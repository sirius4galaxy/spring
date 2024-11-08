
#include <eosio/chain/application.hpp>
#include <eosio/chain/config.hpp>
#include <fc/crypto/public_key.hpp>

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
      config::msig_account_name        = { "eosio.msig"_n };
      config::wrap_account_name        = { "eosio.wrap"_n };

      fc::crypto::config::public_key_legacy_prefix = fc::crypto::config::public_key_eos_prefix;
   }

   scoped_app_tester::scoped_app_tester() {
      compatible_chain_eos();
   }

   custom_scoped_app::custom_scoped_app() {
      (*this)->compatible_chain_eos_handler = compatible_chain_eos;
   }

}// namespace appbase