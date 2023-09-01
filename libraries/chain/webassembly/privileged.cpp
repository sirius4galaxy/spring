#include <eosio/chain/account_object.hpp>
#include <eosio/chain/webassembly/interface.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/protocol_state_object.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/finalizer_set.hpp>
#include <eosio/chain/finalizer_authority.hpp>

#include <fc/io/datastream.hpp>

#include <vector>
#include <set>

namespace eosio { namespace chain { namespace webassembly {

   int interface::is_feature_active( int64_t feature_name ) const { return false; }

   void interface::activate_feature( int64_t feature_name ) const {
      EOS_ASSERT( false, unsupported_feature, "Unsupported Hardfork Detected" );
   }

   void interface::preactivate_feature( legacy_ptr<const digest_type> feature_digest ) {
      EOS_ASSERT(!context.trx_context.is_read_only(), wasm_execution_error, "preactivate_feature not allowed in a readonly transaction");
      context.control.preactivate_feature( *feature_digest, context.trx_context.is_transient() );
   }

   void interface::set_resource_limits( account_name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight ) {
      EOS_ASSERT(!context.trx_context.is_read_only(), wasm_execution_error, "set_resource_limits not allowed in a readonly transaction");
      EOS_ASSERT(ram_bytes >= -1, wasm_execution_error, "invalid value for ram resource limit expected [-1,INT64_MAX]");
      EOS_ASSERT(net_weight >= -1, wasm_execution_error, "invalid value for net resource weight expected [-1,INT64_MAX]");
      EOS_ASSERT(cpu_weight >= -1, wasm_execution_error, "invalid value for cpu resource weight expected [-1,INT64_MAX]");
      if( context.control.get_mutable_resource_limits_manager().set_account_limits(account, ram_bytes, net_weight, cpu_weight, context.trx_context.is_transient()) ) {
         context.trx_context.validate_ram_usage.insert( account );
      }
   }

   void interface::get_resource_limits( account_name account, legacy_ptr<int64_t> ram_bytes, legacy_ptr<int64_t> net_weight, legacy_ptr<int64_t> cpu_weight ) const {
      context.control.get_resource_limits_manager().get_account_limits( account, *ram_bytes, *net_weight, *cpu_weight);
      (void)legacy_ptr<int64_t>(std::move(ram_bytes));
      (void)legacy_ptr<int64_t>(std::move(net_weight));
      (void)legacy_ptr<int64_t>(std::move(cpu_weight));
   }

   int64_t set_proposed_producers_common( apply_context& context, vector<producer_authority> && producers, bool validate_keys ) {
      EOS_ASSERT(producers.size() <= config::max_producers, wasm_execution_error, "Producer schedule exceeds the maximum producer count for this chain");
      EOS_ASSERT( producers.size() > 0
                  || !context.control.is_builtin_activated( builtin_protocol_feature_t::disallow_empty_producer_schedule ),
                  wasm_execution_error,
                  "Producer schedule cannot be empty"
      );

      const size_t num_supported_key_types = context.db.get<protocol_state_object>().num_supported_key_types;

      // check that producers are unique
      std::set<account_name> unique_producers;
      for (const auto& p: producers) {
         EOS_ASSERT( context.is_account(p.producer_name), wasm_execution_error, "producer schedule includes a nonexisting account" );
         std::visit([&p, num_supported_key_types, validate_keys](const auto& a) {
            uint32_t sum_weights = 0;
            std::set<public_key_type> unique_keys;
            for (const auto& kw: a.keys ) {
               EOS_ASSERT( kw.key.which() < num_supported_key_types, unactivated_key_type,
                           "Unactivated key type used in proposed producer schedule");

               if( validate_keys ) {
                  EOS_ASSERT( kw.key.valid(), wasm_execution_error, "producer schedule includes an invalid key" );
               }

               if (std::numeric_limits<uint32_t>::max() - sum_weights <= kw.weight) {
                  sum_weights = std::numeric_limits<uint32_t>::max();
               } else {
                  sum_weights += kw.weight;
               }

               unique_keys.insert(kw.key);
            }

            EOS_ASSERT( a.keys.size() == unique_keys.size(), wasm_execution_error, "producer schedule includes a duplicated key for ${account}", ("account", p.producer_name));
            EOS_ASSERT( a.threshold > 0, wasm_execution_error, "producer schedule includes an authority with a threshold of 0 for ${account}", ("account", p.producer_name));
            EOS_ASSERT( sum_weights >= a.threshold, wasm_execution_error, "producer schedule includes an unsatisfiable authority for ${account}", ("account", p.producer_name));
         }, p.authority);

         unique_producers.insert(p.producer_name);
      }
      EOS_ASSERT( producers.size() == unique_producers.size(), wasm_execution_error, "duplicate producer name in producer schedule" );

      return context.control.set_proposed_producers( std::move(producers) );
   }

   uint32_t interface::get_wasm_parameters_packed( span<char> packed_parameters, uint32_t max_version ) const {
      auto& gpo = context.control.get_global_properties();
      auto& params = gpo.wasm_configuration;
      uint32_t version = std::min( max_version, uint32_t(0) );

      auto s = fc::raw::pack_size( version ) + fc::raw::pack_size( params );
      if ( packed_parameters.size() == 0 )
         return s;

      if ( s <= packed_parameters.size() ) {
         fc::datastream<char*> ds( packed_parameters.data(), s );
         fc::raw::pack(ds, version);
         fc::raw::pack(ds, params);
      }
      return s;
   }
   void interface::set_wasm_parameters_packed( span<const char> packed_parameters ) {
      EOS_ASSERT(!context.trx_context.is_read_only(), wasm_execution_error, "set_wasm_parameters_packed not allowed in a readonly transaction");
      fc::datastream<const char*> ds( packed_parameters.data(), packed_parameters.size() );
      uint32_t version;
      chain::wasm_config cfg;
      fc::raw::unpack(ds, version);
      EOS_ASSERT(version == 0, wasm_config_unknown_version, "set_wasm_parameters_packed: Unknown version: ${version}", ("version", version));
      fc::raw::unpack(ds, cfg);
      cfg.validate();
      context.db.modify( context.control.get_global_properties(),
         [&]( auto& gprops ) {
            gprops.wasm_configuration = cfg;
         }
      );
   }
   int64_t interface::set_proposed_producers( legacy_span<const char> packed_producer_schedule) {
      EOS_ASSERT(!context.trx_context.is_read_only(), wasm_execution_error, "set_proposed_producers not allowed in a readonly transaction");
      fc::datastream<const char*> ds( packed_producer_schedule.data(), packed_producer_schedule.size() );
      std::vector<producer_authority> producers;
      std::vector<legacy::producer_key> old_version;
      fc::raw::unpack(ds, old_version);

      /*
       * Up-convert the producers
       */
      for ( const auto& p : old_version ) {
         producers.emplace_back( producer_authority{ p.producer_name, block_signing_authority_v0{ 1, {{p.block_signing_key, 1}} } } );
      }

      return set_proposed_producers_common( context, std::move(producers), true );
   }

   int64_t interface::set_proposed_producers_ex( uint64_t packed_producer_format, legacy_span<const char> packed_producer_schedule) {
      EOS_ASSERT(!context.trx_context.is_read_only(), wasm_execution_error, "set_proposed_producers_ex not allowed in a readonly transaction");
      if (packed_producer_format == 0) {
         return set_proposed_producers(std::move(packed_producer_schedule));
      } else if (packed_producer_format == 1) {
         fc::datastream<const char*> ds( packed_producer_schedule.data(), packed_producer_schedule.size() );
         vector<producer_authority> producers;

         fc::raw::unpack(ds, producers);
         return set_proposed_producers_common( context, std::move(producers), false);
      } else {
         EOS_THROW(wasm_execution_error, "Producer schedule is in an unknown format!");
      }
   }

   // format for packed_finalizer_set
   struct abi_finalizer_authority {
      std::string              description;
      uint64_t                 fweight = 0; // weight that this finalizer's vote has for meeting fthreshold
      std::array<uint8_t, 144> public_key_g1_jacobian;
   };
   struct abi_finalizer_set {
      uint64_t                             fthreshold = 0;
      std::vector<abi_finalizer_authority> finalizers;
   };

   void interface::set_finalizers(span<const char> packed_finalizer_set) {
      EOS_ASSERT(!context.trx_context.is_read_only(), wasm_execution_error, "set_finalizers not allowed in a readonly transaction");
      fc::datastream<const char*> ds( packed_finalizer_set.data(), packed_finalizer_set.size() );
      abi_finalizer_set abi_finset;
      fc::raw::unpack(ds, abi_finset);

      std::vector<abi_finalizer_authority>& finalizers = abi_finset.finalizers;

      EOS_ASSERT( finalizers.size() <= config::max_finalizers, wasm_execution_error, "Finalizer set exceeds the maximum finalizer count for this chain" );
      EOS_ASSERT( finalizers.size() > 0, wasm_execution_error, "Finalizer set cannot be empty" );

      std::set<fc::crypto::blslib::bls_public_key> unique_finalizer_keys;
      uint64_t f_weight_sum = 0;

      finalizer_set finset;
      finset.fthreshold = abi_finset.fthreshold;
      for (const auto& f: finalizers) {
         EOS_ASSERT( f.description.size() <= config::max_finalizer_description_size, wasm_execution_error,
                     "Finalizer description greater than ${s}", ("s", config::max_finalizer_description_size) );
         f_weight_sum += f.fweight;
         std::optional<bls12_381::g1> pk = bls12_381::g1::fromJacobianBytesLE(f.public_key_g1_jacobian);
         EOS_ASSERT( pk, wasm_execution_error, "Invalid public key for: ${d}", ("d", f.description) );
         finset.finalizers.push_back(finalizer_authority{.description = std::move(f.description), .fweight = f.fweight, .public_key{fc::crypto::blslib::bls_public_key{*pk}}});
         unique_finalizer_keys.insert(finset.finalizers.back().public_key);
      }

      EOS_ASSERT( finalizers.size() == unique_finalizer_keys.size(), wasm_execution_error, "Duplicate finalizer bls key in finalizer set" );
      EOS_ASSERT( finset.fthreshold > f_weight_sum / 2, wasm_execution_error, "Finalizer set threshold cannot be met by finalizer weights" );

      context.control.set_finalizers( finset );
   }

   uint32_t interface::get_blockchain_parameters_packed( legacy_span<char> packed_blockchain_parameters ) const {
      auto& gpo = context.control.get_global_properties();

      auto s = fc::raw::pack_size( gpo.configuration.v0() );
      if( packed_blockchain_parameters.size() == 0 ) return s;

      if ( s <= packed_blockchain_parameters.size() ) {
         fc::datastream<char*> ds( packed_blockchain_parameters.data(), s );
         fc::raw::pack(ds, gpo.configuration.v0());
         return s;
      }
      return 0;
   }

   void interface::set_blockchain_parameters_packed( legacy_span<const char> packed_blockchain_parameters ) {
      EOS_ASSERT(!context.trx_context.is_read_only(), wasm_execution_error, "set_blockchain_parameters_packed not allowed in a readonly transaction");
      fc::datastream<const char*> ds( packed_blockchain_parameters.data(), packed_blockchain_parameters.size() );
      chain::chain_config_v0 cfg;
      fc::raw::unpack(ds, cfg);
      cfg.validate();
      context.db.modify( context.control.get_global_properties(),
         [&]( auto& gprops ) {
              gprops.configuration = cfg;
      });
   }
   
   uint32_t interface::get_parameters_packed( span<const char> packed_parameter_ids, span<char> packed_parameters) const{
      fc::datastream<const char*> ds_ids( packed_parameter_ids.data(), packed_parameter_ids.size() );

      chain::chain_config cfg = context.control.get_global_properties().configuration;
      std::vector<fc::unsigned_int> ids;
      fc::raw::unpack(ds_ids, ids);
      const config_range config_range(cfg, std::move(ids), {context.control});
      
      auto size = fc::raw::pack_size( config_range );
      if( packed_parameters.size() == 0 ) return size;

      EOS_ASSERT(size <= packed_parameters.size(),
                 chain::config_parse_error,
                 "get_parameters_packed: buffer size is smaller than ${size}", ("size", size));
      
      fc::datastream<char*> ds( packed_parameters.data(), size );
      fc::raw::pack( ds, config_range );
      return size;
   }

   void interface::set_parameters_packed( span<const char> packed_parameters ){
      EOS_ASSERT(!context.trx_context.is_read_only(), wasm_execution_error, "set_parameters_packed not allowed in a readonly transaction");
      fc::datastream<const char*> ds( packed_parameters.data(), packed_parameters.size() );

      chain::chain_config cfg = context.control.get_global_properties().configuration;
      config_range config_range(cfg, {context.control});

      fc::raw::unpack(ds, config_range);
      
      config_range.config.validate();
      context.db.modify( context.control.get_global_properties(),
         [&]( auto& gprops ) {
              gprops.configuration = config_range.config;
      });
   }

   bool interface::is_privileged( account_name n ) const {
      return context.db.get<account_metadata_object, by_name>( n ).is_privileged();
   }

   void interface::set_privileged( account_name n, bool is_priv ) {
      EOS_ASSERT(!context.trx_context.is_read_only(), wasm_execution_error, "set_privileged not allowed in a readonly transaction");
      const auto& a = context.db.get<account_metadata_object, by_name>( n );
      context.db.modify( a, [&]( auto& ma ){
         ma.set_privileged( is_priv );
      });
   }
}}} // ns eosio::chain::webassembly

FC_REFLECT(eosio::chain::webassembly::abi_finalizer_authority, (description)(fweight)(public_key_g1_jacobian));
FC_REFLECT(eosio::chain::webassembly::abi_finalizer_set, (fthreshold)(finalizers));