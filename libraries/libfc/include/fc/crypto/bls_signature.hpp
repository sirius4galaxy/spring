#pragma once
#include <fc/static_variant.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/elliptic_r1.hpp>
#include <fc/crypto/elliptic_webauthn.hpp>
#include <fc/crypto/bls_public_key.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <bls12-381/bls12-381.hpp>

namespace fc::crypto::blslib {

   namespace config {
      const std::string bls_signature_prefix = "SIG_BLS_";
   };
   
   class bls_signature
   {
      public:

         bls_signature() = default;
         bls_signature( bls_signature&& ) = default;
         bls_signature( const bls_signature& ) = default;
         explicit bls_signature( const bls12_381::g2& sig ){_sig = sig;}

         // affine non-montgomery base64url with bls_signature_prefix
         explicit bls_signature(const std::string& base64urlstr);

         bls_signature& operator= (const bls_signature& ) = default;

         // affine non-montgomery base64url with bls_signature_prefix
         std::string to_string(const yield_function_t& yield = yield_function_t()) const;

         bool equal( const bls_signature& sig ) const;

         bls12_381::g2 _sig;

   }; // bls_signature

}  // fc::crypto::blslib

namespace fc {
   void to_variant(const crypto::blslib::bls_signature& var, variant& vo, const yield_function_t& yield = yield_function_t());

   void from_variant(const variant& var, crypto::blslib::bls_signature& vo);
} // namespace fc

FC_REFLECT(bls12_381::fp, (d))
FC_REFLECT(bls12_381::fp2, (c0)(c1))
FC_REFLECT(bls12_381::g2, (x)(y)(z))
FC_REFLECT(crypto::blslib::bls_signature, (_sig) )
