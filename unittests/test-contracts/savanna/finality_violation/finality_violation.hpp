#include <eosio/eosio.hpp>
#include <eosio/crypto.hpp>
#include <eosio/crypto_bls_ext.hpp>
#include <eosio/system.hpp>
#include <eosio/transaction.hpp>

#include "../common/savanna.hpp"

using namespace eosio;
using namespace savanna;

CONTRACT finality_violation : public contract {
   public:
      using contract::contract;

      ACTION violaterule1();
      ACTION violaterule2();
      ACTION violaterule3();
      
};