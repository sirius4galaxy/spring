#include <eosio/chain/config.hpp>

namespace eosio::chain::config {

   name system_account_name      = { "eosio"_n };
   name null_account_name        = { "eosio.null"_n };
   name producers_account_name   = { "eosio.prods"_n };
   name eosio_auth_scope         = { "eosio.auth"_n };
   name eosio_all_scope          = { "eosio.all"_n };
   name eosio_any_name           = { "eosio.any"_n };
   name eosio_code_name          = { "eosio.code"_n };
   name token_account_name       = { "eosio.token"_n };
   name msig_account_name        = { "eosio.msig"_n };
   name wrap_account_name        = { "eosio.wrap"_n };

} /// namespace eosio::chain::config
