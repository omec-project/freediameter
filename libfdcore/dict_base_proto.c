/*********************************************************************************************************
* Software License Agreement (BSD License)                                                               *
* Author: Sebastien Decugis <sdecugis@freediameter.net>							 *
*													 *
* Copyright (c) 2013, WIDE Project and NICT								 *
* All rights reserved.											 *
* 													 *
* Redistribution and use of this software in source and binary forms, with or without modification, are  *
* permitted provided that the following conditions are met:						 *
* 													 *
* * Redistributions of source code must retain the above 						 *
*   copyright notice, this list of conditions and the 							 *
*   following disclaimer.										 *
*    													 *
* * Redistributions in binary form must reproduce the above 						 *
*   copyright notice, this list of conditions and the 							 *
*   following disclaimer in the documentation and/or other						 *
*   materials provided with the distribution.								 *
* 													 *
* * Neither the name of the WIDE Project or NICT nor the 						 *
*   names of its contributors may be used to endorse or 						 *
*   promote products derived from this software without 						 *
*   specific prior written permission of WIDE Project and 						 *
*   NICT.												 *
* 													 *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED *
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A *
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 	 *
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 	 *
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR *
* TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF   *
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.								 *
*********************************************************************************************************/

/* Diameter Base protocol definitions.
 */

#include "fdcore-internal.h"

#include <netinet/in.h>
#include <sys/socket.h>

/* The pointer for the global dictionary (initialized from main) */
struct dictionary * fd_g_dict = NULL;



#define CHECK_dict_new( _type, _data, _parent, _ref )				\
	CHECK_FCT(  fd_dict_new( dict, (_type), (_data), (_parent), (_ref))  );

#define CHECK_dict_search( _type, _criteria, _what, _result )					\
	CHECK_FCT(  fd_dict_search( dict, (_type), (_criteria), (_what), (_result), ENOENT)  );

struct local_rules_definition {
	char 			*avp_name;
	enum rule_position	position;
	int 			min;
	int			max;
};

#define RULE_ORDER( _position ) ((((_position) == RULE_FIXED_HEAD) || ((_position) == RULE_FIXED_TAIL)) ? 1 : 0 )

#define PARSE_loc_rules( _rulearray, _parent) {						\
	int __ar;									\
	for (__ar=0; __ar < sizeof(_rulearray) / sizeof((_rulearray)[0]); __ar++) {	\
		struct dict_rule_data __data = { NULL, 					\
			(_rulearray)[__ar].position,					\
			0, 								\
			(_rulearray)[__ar].min,						\
			(_rulearray)[__ar].max};					\
		__data.rule_order = RULE_ORDER(__data.rule_position);			\
		CHECK_FCT(  fd_dict_search( 						\
			dict,								\
			DICT_AVP, 							\
			AVP_BY_NAME, 							\
			(_rulearray)[__ar].avp_name, 					\
			&__data.rule_avp, 0 ) );					\
		if ( !__data.rule_avp ) {						\
			TRACE_DEBUG(INFO, "AVP Not found: '%s'", (_rulearray)[__ar].avp_name );	\
			return ENOENT;							\
		}									\
		CHECK_FCT_DO( fd_dict_new( dict, DICT_RULE, &__data, _parent, NULL),	\
			{								\
				TRACE_DEBUG(INFO, "Error on rule with AVP '%s'",	\
					 (_rulearray)[__ar].avp_name );			\
				return EINVAL;						\
			} );								\
	}										\
}

int fd_dict_base_protocol(struct dictionary * dict)
{
	TRACE_ENTRY("%p", dict);
	CHECK_PARAMS(dict);
	
	/* Vendors section */
	{
		/* The base RFC has no vendor information */
		;
	}
	
	/* Applications section */
	{
		/* base accounting application */
		{
			struct dict_application_data data = {          3, "Diameter Base Accounting" 	};
			CHECK_dict_new( DICT_APPLICATION, &data, NULL, NULL);
		}
		
		/* relay application */
		{
			struct dict_application_data data  = { 0xffffffff, "Relay" 				};
			#if AI_RELAY != 0xffffffff
			#error "AI_RELAY definition mismatch"
			#endif
			CHECK_dict_new( DICT_APPLICATION, &data , NULL, NULL);
		}
	}
	
	/* Derived AVP types section */
	{
		/* Address */
		{
			/*
				Refer RFC6733 section 4.3.1
			*/
			struct dict_type_data data = { AVP_TYPE_OCTETSTRING,	"Address"		, fd_dictfct_Address_interpret	, fd_dictfct_Address_encode,	fd_dictfct_Address_dump	};
			CHECK_dict_new( DICT_TYPE, &data , NULL, NULL);
		}
		
		/* Time */
		{
			/*
				Refer RFC6733 section 4.3.1
			*/
			struct dict_type_data data = { AVP_TYPE_OCTETSTRING,	"Time"			, fd_dictfct_Time_interpret	, fd_dictfct_Time_encode, 	fd_dictfct_Time_dump		};
			CHECK_dict_new( DICT_TYPE, &data , NULL, NULL);
		}
		
		/* UTF8String */
		{
			/*
				Refer RFC6733 section 4.3.1
			*/
			struct dict_type_data data = { AVP_TYPE_OCTETSTRING,	"UTF8String"		, NULL			, NULL	, fd_dictfct_UTF8String_dump	};
			CHECK_dict_new( DICT_TYPE, &data , NULL, NULL);
		}
		
		/* DiameterIdentity */
		{
			/*
				Refer RFC6733 section 4.3.1
			*/
			struct dict_type_data data = { AVP_TYPE_OCTETSTRING,	"DiameterIdentity"	, NULL			, NULL		, fd_dictfct_UTF8String_dump	};
			CHECK_dict_new( DICT_TYPE, &data , NULL, NULL);
		}
		
		/* DiameterURI */
		{
			/*
				Refer RFC6733 section 4.3.1
			*/
			struct dict_type_data data = { AVP_TYPE_OCTETSTRING,	"DiameterURI"		, NULL			, NULL		, fd_dictfct_UTF8String_dump	};
			CHECK_dict_new( DICT_TYPE, &data , NULL, NULL);
		}
		
		/* Enumerated */
		{
			/*
				Refer RFC6733 section 4.3.1
			*/
			
			/* We don't use a generic "Enumerated" type in freeDiameter. Instead, we define
			 * types of the form "Enumerated(<avpname>)" where <avpname> is replaced 
			 * by the name of the AVP to which the type applies.
			 *  Example: Enumerated(Disconnect-Cause)
			 */
			;
		}
		
		/* IPFilterRule */
		{
			/*
				Refer RFC6733 section 4.3.1
			*/
			struct dict_type_data data = { AVP_TYPE_OCTETSTRING,	"IPFilterRule"		, NULL			, NULL		, fd_dictfct_UTF8String_dump	};
			CHECK_dict_new( DICT_TYPE, &data , NULL, NULL);
		}
	}
	
	/* AVP section */
	{
		struct dict_object * Address_type;
		struct dict_object * UTF8String_type;
		struct dict_object * DiameterIdentity_type;
		struct dict_object * DiameterURI_type;
		struct dict_object * Time_type;
		
		CHECK_dict_search( DICT_TYPE, TYPE_BY_NAME, "Address", &Address_type);
		CHECK_dict_search( DICT_TYPE, TYPE_BY_NAME, "UTF8String", &UTF8String_type);
		CHECK_dict_search( DICT_TYPE, TYPE_BY_NAME, "DiameterIdentity", &DiameterIdentity_type);
		CHECK_dict_search( DICT_TYPE, TYPE_BY_NAME, "DiameterURI", &DiameterURI_type);
		CHECK_dict_search( DICT_TYPE, TYPE_BY_NAME, "Time", &Time_type);
		
		/* Vendor-Id */
		{
			/*
				Refer RFC6733 section 5.3.3
			*/
			struct dict_avp_data data = { 
					266, 					/* Code */
					#if AC_VENDOR_ID != 266
					#error "AC_VENDOR_ID definition mismatch"
					#endif
					0, 					/* Vendor */
					"Vendor-Id", 				/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY, 			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data, NULL, NULL);
		}
		
		/* Firmware-Revision */
		{
			/*
				Refer RFC6733 section 5.3.4
			*/
			struct dict_avp_data data = { 
					267, 					/* Code */
					#if AC_FIRMWARE_REVISION != 267
					#error "AC_FIRMWARE_REVISION definition mismatch"
					#endif
					0, 					/* Vendor */
					"Firmware-Revision", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					0,		 			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Host-IP-Address */
		{
			/*
				Refer RFC6733 section 5.3.5
			*/
			struct dict_avp_data data = { 
					257, 					/* Code */
					#if AC_HOST_IP_ADDRESS != 257
					#error "AC_HOST_IP_ADDRESS definition mismatch"
					#endif
					0, 					/* Vendor */
					"Host-IP-Address", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , Address_type, NULL);
		}
		
		/* Supported-Vendor-Id */
		{
			/*
				Refer RFC6733 section 5.3.6
			*/
			struct dict_avp_data data = { 
					265, 					/* Code */
					#if AC_SUPPORTED_VENDOR_ID != 265
					#error "AC_SUPPORTED_VENDOR_ID definition mismatch"
					#endif
					0, 					/* Vendor */
					"Supported-Vendor-Id", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Product-Name */
		{
			/*
				Refer RFC6733 section 5.3.7
			*/
			struct dict_avp_data data = { 
					269, 					/* Code */
					#if AC_PRODUCT_NAME != 269
					#error "AC_PRODUCT_NAME definition mismatch"
					#endif
					0, 					/* Vendor */
					"Product-Name", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					0,					/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , UTF8String_type, NULL);
		}
		
		/* Disconnect-Cause */
		{
			/*
				Refer RFC6733 section 5.4.3
			*/
			struct dict_object 	* 	type;
			struct dict_type_data 		tdata = { AVP_TYPE_INTEGER32,	"Enumerated(Disconnect-Cause)"	, NULL, NULL, NULL };
			struct dict_enumval_data 	t_0 = { "REBOOTING", 			{ .i32 = 0 }};
			struct dict_enumval_data 	t_1 = { "BUSY", 			{ .i32 = 1 }};
			struct dict_enumval_data 	t_2 = { "DO_NOT_WANT_TO_TALK_TO_YOU", 	{ .i32 = 2 }};
			struct dict_avp_data 		data = { 
					273, 					/* Code */
					#if AC_DISCONNECT_CAUSE != 273
					#error "AC_DISCONNECT_CAUSE definition mismatch"
					#endif
					0, 					/* Vendor */
					"Disconnect-Cause", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_INTEGER32 			/* base type of data */
					};
			/* Create the Enumerated type, and then the AVP */
			CHECK_dict_new( DICT_TYPE, &tdata , NULL, &type);
			CHECK_dict_new( DICT_ENUMVAL, &t_0 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_1 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_2 , type, NULL);
			CHECK_dict_new( DICT_AVP, &data , type, NULL);
		}
		
		/* Origin-Host */
		{
			/*
				Refer RFC6733 section 6.3
			*/
			struct dict_avp_data data = { 
					264, 					/* Code */
					#if AC_ORIGIN_HOST != 264
					#error "AC_ORIGIN_HOST definition mismatch"
					#endif
					0, 					/* Vendor */
					"Origin-Host", 				/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , DiameterIdentity_type, NULL);
		}
		
		/* Origin-Realm */
		{
			/*
				Refer RFC6733 section 6.4
			*/
			struct dict_avp_data data = { 
					296, 					/* Code */
					#if AC_ORIGIN_REALM != 296
					#error "AC_ORIGIN_REALM definition mismatch"
					#endif
					0, 					/* Vendor */
					"Origin-Realm", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , DiameterIdentity_type, NULL);
		}
		
		/* Destination-Host */
		{
			/*
				Refer RFC6733 section 6.5
			*/
			struct dict_avp_data data = { 
					293, 					/* Code */
					#if AC_DESTINATION_HOST != 293
					#error "AC_DESTINATION_HOST definition mismatch"
					#endif
					0, 					/* Vendor */
					"Destination-Host", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , DiameterIdentity_type, NULL);
		}
		
		/* Destination-Realm */
		{
			/*
				Refer RFC6733 section 6.6
			*/
			struct dict_avp_data data = { 
					283, 					/* Code */
					#if AC_DESTINATION_REALM != 283
					#error "AC_DESTINATION_REALM definition mismatch"
					#endif
					0, 					/* Vendor */
					"Destination-Realm", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , DiameterIdentity_type, NULL);
		}
		
		/* Route-Record */
		{
			/*
				Refer RFC6733 section 6.7.1
			*/
			struct dict_avp_data data = { 
					282, 					/* Code */
					#if AC_ROUTE_RECORD != 282
					#error "AC_ROUTE_RECORD definition mismatch"
					#endif
					0, 					/* Vendor */
					"Route-Record", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , DiameterIdentity_type, NULL);
		}
		
		/* Proxy-Host */
		{
			/*
				Refer RFC6733 section 6.7.3
			*/
			struct dict_avp_data adata = { 
					280, 					/* Code */
					#if AC_PROXY_HOST != 280
					#error "AC_PROXY_HOST definition mismatch"
					#endif
					0, 					/* Vendor */
					"Proxy-Host", 				/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &adata , DiameterIdentity_type, NULL);
		}
		
		/* Proxy-State */
		{
			/*
				Refer RFC6733 section 6.7.4
			*/
			struct dict_avp_data adata = { 
					33, 					/* Code */
					#if AC_PROXY_STATE != 33
					#error "AC_PROXY_STATE definition mismatch"
					#endif
					0, 					/* Vendor */
					"Proxy-State", 				/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &adata , NULL, NULL);
		}
			
		/* Proxy-Info */
		{
			/*
				Refer RFC6733 section 6.7.2
			*/
			struct dict_object * avp;
			struct dict_avp_data data = { 
					284, 					/* Code */
					#if AC_PROXY_INFO != 284
					#error "AC_PROXY_INFO definition mismatch"
					#endif
					0, 					/* Vendor */
					"Proxy-Info", 				/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_GROUPED 			/* base type of data */
					};
			struct local_rules_definition rules[] = 
						{ 	 {  "Proxy-Host", 			RULE_REQUIRED, -1, 1 }
							,{  "Proxy-State",			RULE_REQUIRED, -1, 1 }
						};
			
			CHECK_dict_new( DICT_AVP, &data , NULL, &avp);
			PARSE_loc_rules( rules, avp );
		}
		
		/* Auth-Application-Id */
		{
			/*
				Refer RFC6733 section 6.8
			*/
			struct dict_avp_data data = { 
					258, 					/* Code */
					#if AC_AUTH_APPLICATION_ID != 258
					#error "AC_AUTH_APPLICATION_ID definition mismatch"
					#endif
					0, 					/* Vendor */
					"Auth-Application-Id", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Acct-Application-Id */
		{
			/*
				Refer RFC6733 section 6.9
			*/
			struct dict_avp_data data = { 
					259, 					/* Code */
					#if AC_ACCT_APPLICATION_ID != 259
					#error "AC_ACCT_APPLICATION_ID definition mismatch"
					#endif
					0, 					/* Vendor */
					"Acct-Application-Id", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Inband-Security-Id */
		{
			/*
				Refer RFC6733 section 6.10
			*/
			
			/* Although the RFC does not specify an "Enumerated" type here, we go forward and create one.
			 * This is the reason for the "*" in the type name
			 */
			
			struct dict_object 	* 	type;
			struct dict_type_data	 	tdata = { AVP_TYPE_UNSIGNED32,	"Enumerated(Inband-Security-Id)"	, NULL, NULL, NULL };
			struct dict_enumval_data 	t_0 = { "NO_INBAND_SECURITY", 		{ .u32 = ACV_ISI_NO_INBAND_SECURITY }};
			struct dict_enumval_data 	t_1 = { "TLS", 			{ .u32 = ACV_ISI_TLS }};
			struct dict_avp_data 		data = { 
					299, 					/* Code */
					#if AC_INBAND_SECURITY_ID != 299
					#error "AC_INBAND_SECURITY_ID definition mismatch"
					#endif
					0, 					/* Vendor */
					"Inband-Security-Id", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			/* Create the Enumerated type, and then the AVP */
			CHECK_dict_new( DICT_TYPE, &tdata , NULL, &type);
			CHECK_dict_new( DICT_ENUMVAL, &t_0 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_1 , type, NULL);
			CHECK_dict_new( DICT_AVP, &data , type, NULL);
		}
		
		/* Vendor-Specific-Application-Id */
		{
			/*
				Refer RFC6733 section 6.11
			*/
			struct dict_object 	* avp;
			struct dict_avp_data	  data = { 
					260, 					/* Code */
					#if AC_VENDOR_SPECIFIC_APPLICATION_ID != 260
					#error "AC_VENDOR_SPECIFIC_APPLICATION_ID definition mismatch"
					#endif
					0, 					/* Vendor */
					"Vendor-Specific-Application-Id", 	/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_GROUPED 			/* base type of data */
					};
					
			struct local_rules_definition rules[] = 
						{
#ifndef WORKAROUND_ACCEPT_INVALID_VSAI
							/* ABNF from RFC6733 */
						 	 {  "Vendor-Id", 			RULE_REQUIRED, -1, 1 }
#else /* WORKAROUND_ACCEPT_INVALID_VSAI */
							/* ABNF from RFC3588 (including erratum, because original text is nonsense) */
						 	 {  "Vendor-Id", 			RULE_REQUIRED, -1, -1}
#endif /* WORKAROUND_ACCEPT_INVALID_VSAI */
							,{  "Auth-Application-Id",		RULE_OPTIONAL, -1, 1 }
							,{  "Acct-Application-Id",		RULE_OPTIONAL, -1, 1 }
						};
			
			/* Create the grouped AVP */
			CHECK_dict_new( DICT_AVP, &data , NULL, &avp);
			PARSE_loc_rules( rules, avp );
			
		}
		
		/* Redirect-Host */
		{
			/*
				Refer RFC6733 section 6.12
			*/
			struct dict_avp_data data = { 
					292, 					/* Code */
					#if AC_REDIRECT_HOST != 292
					#error "AC_REDIRECT_HOST definition mismatch"
					#endif
					0, 					/* Vendor */
					"Redirect-Host", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , DiameterURI_type, NULL);
		}
		
		/* Redirect-Host-Usage */
		{
			/*
				Refer RFC6733 section 6.13
			*/
			struct dict_object 	* 	type;
			struct dict_type_data	 	tdata = { AVP_TYPE_INTEGER32,	"Enumerated(Redirect-Host-Usage)"	, NULL, NULL, NULL };
			struct dict_enumval_data 	t_0 = { "DONT_CACHE", 			{ .i32 = 0 }};
			struct dict_enumval_data 	t_1 = { "ALL_SESSION", 			{ .i32 = 1 }};
			struct dict_enumval_data 	t_2 = { "ALL_REALM", 			{ .i32 = 2 }};
			struct dict_enumval_data 	t_3 = { "REALM_AND_APPLICATION", 	{ .i32 = 3 }};
			struct dict_enumval_data 	t_4 = { "ALL_APPLICATION", 		{ .i32 = 4 }};
			struct dict_enumval_data 	t_5 = { "ALL_HOST", 			{ .i32 = 5 }};
			struct dict_enumval_data 	t_6 = { "ALL_USER", 			{ .i32 = 6 }};
			struct dict_avp_data 		data = { 
					261, 					/* Code */
					#if AC_REDIRECT_HOST_USAGE != 261
					#error "AC_REDIRECT_HOST_USAGE definition mismatch"
					#endif
					0, 					/* Vendor */
					"Redirect-Host-Usage", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_INTEGER32 			/* base type of data */
					};
			/* Create the Enumerated type, and then the AVP */
			CHECK_dict_new( DICT_TYPE, &tdata , NULL, &type);
			CHECK_dict_new( DICT_ENUMVAL, &t_0 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_1 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_2 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_3 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_4 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_5 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_6 , type, NULL);
			CHECK_dict_new( DICT_AVP, &data , type, NULL);
		}
		
		/* Redirect-Max-Cache-Time */
		{
			/*
				Refer RFC6733 section 6.14
			*/
			struct dict_avp_data data = { 
					262, 					/* Code */
					#if AC_REDIRECT_MAX_CACHE_TIME != 262
					#error "AC_REDIRECT_MAX_CACHE_TIME definition mismatch"
					#endif
					0, 					/* Vendor */
					"Redirect-Max-Cache-Time", 		/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Result-Code */
		{
			/*
				Refer RFC6733 section 7.1
			*/
			
			/* Although the RFC does not specify an "Enumerated" type here, we go forward and create one.
			 * This is the reason for the "*" in the type name
			 */
			struct dict_object * 	type;
			struct dict_type_data 	tdata = { AVP_TYPE_UNSIGNED32,	"Enumerated(Result-Code)"	, NULL, NULL, NULL };
			struct dict_avp_data 	data = { 
					268, 					/* Code */
					#if AC_RESULT_CODE != 268
					#error "AC_RESULT_CODE definition mismatch"
					#endif
					0, 					/* Vendor */
					"Result-Code", 				/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			/* Create the Enumerated type, and then the AVP */
			CHECK_dict_new( DICT_TYPE, &tdata , NULL, &type);
			CHECK_dict_new( DICT_AVP, &data , type, NULL);
			
			/* Informational */
			{
				/* 1001 */
				{
					/*
						Refer RFC6733 section 7.1.1
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_MULTI_ROUND_AUTH", 	{ .u32 = ER_DIAMETER_MULTI_ROUND_AUTH }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
			}
			/* Success */
			{
				/* 2001 */
				{
					/*
						Refer RFC6733 section 7.1.2
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_SUCCESS", 		{ .u32 = ER_DIAMETER_SUCCESS }};
					#if ER_DIAMETER_SUCCESS != 2001
					#error "ER_DIAMETER_SUCCESS definition mismatch"
					#endif
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 2002 */
				{
					/*
						Refer RFC6733 section 7.1.2
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_LIMITED_SUCCESS", 	{ .u32 = ER_DIAMETER_LIMITED_SUCCESS }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
			}
			/* Protocol Errors */
			{
				/* 3001 -- might be changed to 5xxx soon */
				{
					/*
						Refer RFC6733 section 7.1.3
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_COMMAND_UNSUPPORTED", 	{ .u32 = ER_DIAMETER_COMMAND_UNSUPPORTED }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 3002 */
				{
					/*
						Refer RFC6733 section 7.1.3
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_UNABLE_TO_DELIVER", 	{ .u32 = ER_DIAMETER_UNABLE_TO_DELIVER }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 3003 */
				{
					/*
						Refer RFC6733 section 7.1.3
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_REALM_NOT_SERVED", 	{ .u32 = ER_DIAMETER_REALM_NOT_SERVED }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 3004 */
				{
					/*
						Refer RFC6733 section 7.1.3
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_TOO_BUSY", 		{ .u32 = ER_DIAMETER_TOO_BUSY }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 3005 */
				{
					/*
						Refer RFC6733 section 7.1.3
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_LOOP_DETECTED", 	{ .u32 = ER_DIAMETER_LOOP_DETECTED }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 3006 */
				{
					/*
						Refer RFC6733 section 7.1.3
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_REDIRECT_INDICATION", 	{ .u32 = ER_DIAMETER_REDIRECT_INDICATION }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 3007 */
				{
					/*
						Refer RFC6733 section 7.1.3
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_APPLICATION_UNSUPPORTED",	{ .u32 = ER_DIAMETER_APPLICATION_UNSUPPORTED }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 3008 -- will change to 5xxx soon */
				{
					/*
						Refer RFC6733 section 7.1.3
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_INVALID_HDR_BITS", 	{ .u32 = ER_DIAMETER_INVALID_HDR_BITS }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 3009 -- will change to 5xxx soon */
				{
					/*
						Refer RFC6733 section 7.1.3
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_INVALID_AVP_BITS", 	{ .u32 = ER_DIAMETER_INVALID_AVP_BITS }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 3010  -- will change to 5xxx soon */
				{
					/*
						 Refer RFC6733 section 7.1.3
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_UNKNOWN_PEER", 	{ .u32 = ER_DIAMETER_UNKNOWN_PEER }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
			}
			/* Transient Failures */
			{
				/* 4001 */
				{
					/*
						Refer RFC6733 section 7.1.4
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_AUTHENTICATION_REJECTED", 	{ .u32 = ER_DIAMETER_AUTHENTICATION_REJECTED }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 4002 */
				{
					/*
						Refer RFC6733 section 7.1.4
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_OUT_OF_SPACE", 		{ .u32 = ER_DIAMETER_OUT_OF_SPACE }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 4003 */
				{
					/*
						Refer RFC6733 section 7.1.4
					*/
					struct dict_enumval_data 	error_code = { "ELECTION_LOST", 			{ .u32 = ER_ELECTION_LOST }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
			}
			/* Permanent Failures */
			{
				/* 5001 */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_AVP_UNSUPPORTED", 	{ .u32 = ER_DIAMETER_AVP_UNSUPPORTED }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5002 */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_UNKNOWN_SESSION_ID", 	{ .u32 = ER_DIAMETER_UNKNOWN_SESSION_ID }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5003 */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_AUTHORIZATION_REJECTED",{ .u32 = ER_DIAMETER_AUTHORIZATION_REJECTED }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5004 */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_INVALID_AVP_VALUE",	{ .u32 = ER_DIAMETER_INVALID_AVP_VALUE }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5005 */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_MISSING_AVP",		{ .u32 = ER_DIAMETER_MISSING_AVP }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5006 */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_RESOURCES_EXCEEDED",	{ .u32 = ER_DIAMETER_RESOURCES_EXCEEDED }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5007 */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_CONTRADICTING_AVPS",	{ .u32 = ER_DIAMETER_CONTRADICTING_AVPS }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5008 */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_AVP_NOT_ALLOWED",	{ .u32 = ER_DIAMETER_AVP_NOT_ALLOWED }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5009 */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_AVP_OCCURS_TOO_MANY_TIMES",{ .u32 = ER_DIAMETER_AVP_OCCURS_TOO_MANY_TIMES }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5010 */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_NO_COMMON_APPLICATION",{ .u32 = ER_DIAMETER_NO_COMMON_APPLICATION }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5011 */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_UNSUPPORTED_VERSION",	{ .u32 = ER_DIAMETER_UNSUPPORTED_VERSION }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5012 */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_UNABLE_TO_COMPLY",	{ .u32 = ER_DIAMETER_UNABLE_TO_COMPLY }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5013 -- will change to 3xxx */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_INVALID_BIT_IN_HEADER", 	{ .u32 = ER_DIAMETER_INVALID_BIT_IN_HEADER }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5014 */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_INVALID_AVP_LENGTH",	{ .u32 = ER_DIAMETER_INVALID_AVP_LENGTH }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5015 -- will change to 3xxx */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_INVALID_MESSAGE_LENGTH", 	{ .u32 = ER_DIAMETER_INVALID_MESSAGE_LENGTH }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5016 */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_INVALID_AVP_BIT_COMBO", 	{ .u32 = ER_DIAMETER_INVALID_AVP_BIT_COMBO }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
				/* 5017 */
				{
					/*
						Refer RFC6733 section 7.1.5
					*/
					struct dict_enumval_data 	error_code = { "DIAMETER_NO_COMMON_SECURITY",	{ .u32 = ER_DIAMETER_NO_COMMON_SECURITY }};
					CHECK_dict_new( DICT_ENUMVAL, &error_code , type, NULL);
				}
			}
		}
		
		/* Error-Message */
		{
			/*
				Refer RFC6733 section 7.3
			*/
			struct dict_avp_data data = { 
					281, 					/* Code */
					#if AC_ERROR_MESSAGE != 281
					#error "AC_ERROR_MESSAGE definition mismatch"
					#endif
					0, 					/* Vendor */
					"Error-Message", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					0,					/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , UTF8String_type, NULL);
		}
		
		/* Error-Reporting-Host */
		{
			/*
				Refer RFC6733 section 7.4
			*/
			struct dict_avp_data data = { 
					294, 					/* Code */
					#if AC_ERROR_REPORTING_HOST != 294
					#error "AC_ERROR_REPORTING_HOST definition mismatch"
					#endif
					0, 					/* Vendor */
					"Error-Reporting-Host", 		/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					0,					/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , DiameterIdentity_type, NULL);
		}
		
		/* Failed-AVP */
		{
			/*
				Refer RFC6733 section 7.5
			*/
			struct dict_avp_data data = { 
					279, 					/* Code */
					#if AC_FAILED_AVP != 279
					#error "AC_FAILED_AVP definition mismatch"
					#endif
					0, 					/* Vendor */
					"Failed-AVP", 				/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_GROUPED 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Experimental-Result */
		{
			/*
				Refer RFC6733 section 7.6
			*/
			struct dict_avp_data data = { 
					297, 					/* Code */
					0, 					/* Vendor */
					"Experimental-Result", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_GROUPED 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Experimental-Result-Code */
		{
			/*
				Refer RFC6733 section 7.7
			*/
			/* Although the RFC does not specify an "Enumerated" type here, we go forward and create one.
			 * This is the reason for the "*" in the type name. Vendors will have to define their values.
			 */
			struct dict_object * 	type;
			struct dict_type_data 	tdata = { AVP_TYPE_UNSIGNED32,	"Enumerated(Experimental-Result-Code)"	, NULL, NULL, NULL };
			struct dict_avp_data 	data = { 
					298, 					/* Code */
					0, 					/* Vendor */
					"Experimental-Result-Code", 		/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			
			CHECK_dict_new( DICT_TYPE, &tdata , NULL, &type);
			CHECK_dict_new( DICT_AVP, &data , type, NULL);
		}
		
		/* Auth-Request-Type */
		{
			/*
				Refer RFC6733 section 8.7
			*/
			struct dict_object 	* 	type;
			struct dict_type_data 		tdata = { AVP_TYPE_INTEGER32,	"Enumerated(Auth-Request-Type)"	, NULL, NULL, NULL };
			struct dict_enumval_data 	t_1 = { "AUTHENTICATE_ONLY", 		{ .i32 = 1 }};
			struct dict_enumval_data 	t_2 = { "AUTHORIZE_ONLY", 		{ .i32 = 2 }};
			struct dict_enumval_data 	t_3 = { "AUTHORIZE_AUTHENTICATE", 	{ .i32 = 3 }};
			struct dict_avp_data 	data = { 
					274, 					/* Code */
					0, 					/* Vendor */
					"Auth-Request-Type", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_INTEGER32 			/* base type of data */
					};
			/* Create the Enumerated type, and then the AVP */
			CHECK_dict_new( DICT_TYPE, &tdata , NULL, &type);
			CHECK_dict_new( DICT_ENUMVAL, &t_1 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_2 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_3 , type, NULL);
			CHECK_dict_new( DICT_AVP, &data , type, NULL);
		}
		
		/* Session-Id */
		{
			/*
				Refer RFC6733 section 8.8
			*/
			struct dict_avp_data data = { 
					263, 					/* Code */
					#if AC_SESSION_ID != 263
					#error "AC_SESSION_ID definition mismatch"
					#endif
					0, 					/* Vendor */
					"Session-Id", 				/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , UTF8String_type, NULL);
		}
		
		/* Authorization-Lifetime */
		{
			/*
				Refer RFC6733 section 8.9
			*/
			struct dict_avp_data data = { 
					291, 					/* Code */
					0, 					/* Vendor */
					"Authorization-Lifetime", 		/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Auth-Grace-Period */
		{
			/*
				Refer RFC6733 section 8.10
			*/
			struct dict_avp_data data = { 
					276, 					/* Code */
					0, 					/* Vendor */
					"Auth-Grace-Period", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Auth-Session-State */
		{
			/*
				Refer RFC6733 section 8.11
			*/
			struct dict_object 	* 	type;
			struct dict_type_data	 	tdata = { AVP_TYPE_INTEGER32,	"Enumerated(Auth-Session-State)"	, NULL, NULL, NULL };
			struct dict_enumval_data 	t_0 = { "STATE_MAINTAINED", 		{ .i32 = 0 }};
			struct dict_enumval_data 	t_1 = { "NO_STATE_MAINTAINED", 		{ .i32 = 1 }};
			struct dict_avp_data	 	data = { 
					277, 					/* Code */
					0, 					/* Vendor */
					"Auth-Session-State", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_INTEGER32 			/* base type of data */
					};
			/* Create the Enumerated type, and then the AVP */
			CHECK_dict_new( DICT_TYPE, &tdata , NULL, &type);
			CHECK_dict_new( DICT_ENUMVAL, &t_0 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_1 , type, NULL);
			CHECK_dict_new( DICT_AVP, &data , type, NULL);
		}
		
		/* Re-Auth-Request-Type */
		{
			/*
				Refer RFC6733 section 8.12
			*/
			struct dict_object 	* 	type;
			struct dict_type_data	 	tdata = { AVP_TYPE_INTEGER32,	"Enumerated(Re-Auth-Request-Type)"	, NULL, NULL, NULL };
			struct dict_enumval_data 	t_0 = { "AUTHORIZE_ONLY", 		{ .i32 = 0 }};
			struct dict_enumval_data 	t_1 = { "AUTHORIZE_AUTHENTICATE",	{ .i32 = 1 }};
			struct dict_avp_data	 	data = { 
					285, 					/* Code */
					0, 					/* Vendor */
					"Re-Auth-Request-Type",			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_INTEGER32 			/* base type of data */
					};
			/* Create the Enumerated type, and then the AVP */
			CHECK_dict_new( DICT_TYPE, &tdata , NULL, &type);
			CHECK_dict_new( DICT_ENUMVAL, &t_0 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_1 , type, NULL);
			CHECK_dict_new( DICT_AVP, &data , type, NULL);
		}
		
		/* Session-Timeout */
		{
			/*
				Refer RFC6733 section 8.13
			*/
			struct dict_avp_data data = { 
					27, 					/* Code */
					0, 					/* Vendor */
					"Session-Timeout", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* User-Name */
		{
			/*
				Refer RFC6733 section 8.14
			*/
			struct dict_avp_data data = { 
					1, 					/* Code */
					0, 					/* Vendor */
					"User-Name", 				/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , UTF8String_type, NULL);
		}
		
		/* Termination-Cause */
		{
			/*
				Refer RFC3588 section 8.15
			*/
			struct dict_object 	* 	type;
			struct dict_type_data	 	tdata = { AVP_TYPE_INTEGER32,	"Enumerated(Termination-Cause)"	, NULL, NULL, NULL };
			struct dict_enumval_data 	t_1 = { "DIAMETER_LOGOUT",			{ .i32 = 1 }};
			struct dict_enumval_data 	t_2 = { "DIAMETER_SERVICE_NOT_PROVIDED", 	{ .i32 = 2 }};
			struct dict_enumval_data 	t_3 = { "DIAMETER_BAD_ANSWER",			{ .i32 = 3 }};
			struct dict_enumval_data 	t_4 = { "DIAMETER_ADMINISTRATIVE", 		{ .i32 = 4 }};
			struct dict_enumval_data 	t_5 = { "DIAMETER_LINK_BROKEN",			{ .i32 = 5 }};
			struct dict_enumval_data 	t_6 = { "DIAMETER_AUTH_EXPIRED", 		{ .i32 = 6 }};
			struct dict_enumval_data 	t_7 = { "DIAMETER_USER_MOVED",			{ .i32 = 7 }};
			struct dict_enumval_data 	t_8 = { "DIAMETER_SESSION_TIMEOUT", 		{ .i32 = 8 }};
			struct dict_avp_data 	data = { 
					295, 					/* Code */
					0, 					/* Vendor */
					"Termination-Cause",			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_INTEGER32 			/* base type of data */
					};
			/* Create the Enumerated type, and then the AVP */
			CHECK_dict_new( DICT_TYPE, &tdata , NULL, &type);
			CHECK_dict_new( DICT_ENUMVAL, &t_1 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_2 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_3 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_4 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_5 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_6 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_7 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_8 , type, NULL);
			CHECK_dict_new( DICT_AVP, &data , type, NULL);
		}
		
		/* Origin-State-Id */
		{
			/*
				Refer RFC6733 section 8.16
			*/
			struct dict_avp_data data = { 
					278, 					/* Code */
					0, 					/* Vendor */
					"Origin-State-Id", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Session-Binding */
		{
			/*
				Refer RFC6733 section 8.17
			*/
			
			/* Although the RFC does not specify an "Enumerated" type here, we go forward and create one.
			 * This is the reason for the "*" in the type name
			 * The actual values of the AVP will not always be defined here, but at least it can be used in some cases.
			 *  ... maybe the code will be changed later to support bitfields AVP ...
			 */
			
			struct dict_object 	* 	type;
			struct dict_type_data	 	tdata = { AVP_TYPE_UNSIGNED32,	"Enumerated(Session-Binding)"	, NULL, NULL, NULL };
			struct dict_enumval_data 	t_1 = { "RE_AUTH", 		{ .u32 = 1 }};
			struct dict_enumval_data 	t_2 = { "STR", 			{ .u32 = 2 }};
			struct dict_enumval_data 	t_4 = { "ACCOUNTING", 		{ .u32 = 4 }};
			struct dict_avp_data 	data = { 
					270, 					/* Code */
					0, 					/* Vendor */
					"Session-Binding", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			/* Create the Enumerated type, and then the AVP */
			CHECK_dict_new( DICT_TYPE, &tdata , NULL, &type);
			CHECK_dict_new( DICT_ENUMVAL, &t_1 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_2 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_4 , type, NULL);
			CHECK_dict_new( DICT_AVP, &data , type, NULL);
		}
		
		/* Session-Server-Failover */
		{
			/*
				Refer RFC6733 section 8.18
			*/
			struct dict_object  	* 	type;
			struct dict_type_data	 	tdata = { AVP_TYPE_INTEGER32,	"Enumerated(Session-Server-Failover)"	, NULL, NULL, NULL };
			struct dict_enumval_data 	t_0 = { "REFUSE_SERVICE", 		{ .i32 = 0 }};
			struct dict_enumval_data 	t_1 = { "TRY_AGAIN",			{ .i32 = 1 }};
			struct dict_enumval_data 	t_2 = { "ALLOW_SERVICE", 		{ .i32 = 2 }};
			struct dict_enumval_data 	t_3 = { "TRY_AGAIN_ALLOW_SERVICE",	{ .i32 = 3 }};
			struct dict_avp_data	 	data = { 
					271, 					/* Code */
					0, 					/* Vendor */
					"Session-Server-Failover",		/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_INTEGER32 			/* base type of data */
					};
			/* Create the Enumerated type, and then the AVP */
			CHECK_dict_new( DICT_TYPE, &tdata , NULL, &type);
			CHECK_dict_new( DICT_ENUMVAL, &t_0 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_1 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_2 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_3 , type, NULL);
			CHECK_dict_new( DICT_AVP, &data , type, NULL);
		}
		
		/* Multi-Round-Time-Out */
		{
			/*
				Refer RFC6733 section 8.19
			*/
			struct dict_avp_data data = { 
					272, 					/* Code */
					0, 					/* Vendor */
					"Multi-Round-Time-Out", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Class */
		{
			/*
				Refer RFC6733 section 8.20
			*/
			struct dict_avp_data data = { 
					25, 					/* Code */
					0, 					/* Vendor */
					"Class", 				/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Event-Timestamp */
		{
			/*
				Refer RFC6733 section 8.21
			*/
			struct dict_avp_data data = { 
					55, 					/* Code */
					0, 					/* Vendor */
					"Event-Timestamp", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , Time_type, NULL);
		}

				
		/* Accounting-Record-Type */
		{
			/*
				Refer RFC6733 section 9.8.1
			*/
			struct dict_object 	* 	type;
			struct dict_type_data	  	tdata = { AVP_TYPE_INTEGER32,	"Enumerated(Accounting-Record-Type)"	, NULL, NULL, NULL };
			struct dict_enumval_data 	t_1 = { "EVENT_RECORD",			{ .i32 = 1 }};
			struct dict_enumval_data 	t_2 = { "START_RECORD", 		{ .i32 = 2 }};
			struct dict_enumval_data 	t_3 = { "INTERIM_RECORD",		{ .i32 = 3 }};
			struct dict_enumval_data 	t_4 = { "STOP_RECORD", 			{ .i32 = 4 }};
			struct dict_avp_data 	data = { 
					480, 					/* Code */
					0, 					/* Vendor */
					"Accounting-Record-Type",		/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_INTEGER32 			/* base type of data */
					};
			/* Create the Enumerated type, and then the AVP */
			CHECK_dict_new( DICT_TYPE, &tdata , NULL, &type);
			CHECK_dict_new( DICT_ENUMVAL, &t_1 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_2 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_3 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_4 , type, NULL);
			CHECK_dict_new( DICT_AVP, &data , type, NULL);
		}
		
		/* Acct-Interim-Interval */
		{
			/*
				Refer RFC6733 section 9.8.2
			*/
			struct dict_avp_data data = { 
					85, 					/* Code */
					0, 					/* Vendor */
					"Acct-Interim-Interval", 		/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Accounting-Record-Number */
		{
			/*
				Refer RFC6733 section 9.8.3
			*/
			struct dict_avp_data data = { 
					485, 					/* Code */
					0, 					/* Vendor */
					"Accounting-Record-Number", 		/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED32 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Acct-Session-Id */
		{
			/*
				Refer RFC6733 section 9.8.4
			*/
			struct dict_avp_data data = { 
					44, 					/* Code */
					0, 					/* Vendor */
					"Acct-Session-Id", 			/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Acct-Multi-Session-Id */
		{
			/*
				Refer RFC6733 section 9.8.5
			*/
			struct dict_avp_data data = { 
					50, 					/* Code */
					0, 					/* Vendor */
					"Acct-Multi-Session-Id", 		/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_OCTETSTRING 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , UTF8String_type, NULL);
		}
		
		/* Accounting-Sub-Session-Id */
		{
			/*
				Refer RFC6733 section 9.8.6
			*/
			struct dict_avp_data data = { 
					287, 					/* Code */
					0, 					/* Vendor */
					"Accounting-Sub-Session-Id", 		/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_UNSIGNED64 			/* base type of data */
					};
			CHECK_dict_new( DICT_AVP, &data , NULL, NULL);
		}
		
		/* Accounting-Realtime-Required */
		{
			/*
				Refer RFC6733 section 9.8.7
			*/
			struct dict_object  	* 	type;
			struct dict_type_data	 	tdata = { AVP_TYPE_INTEGER32,	"Enumerated(Accounting-Realtime-Required)"	, NULL, NULL, NULL };
			struct dict_enumval_data 	t_1 = { "DELIVER_AND_GRANT",		{ .i32 = 1 }};
			struct dict_enumval_data 	t_2 = { "GRANT_AND_STORE", 		{ .i32 = 2 }};
			struct dict_enumval_data 	t_3 = { "GRANT_AND_LOSE",		{ .i32 = 3 }};
			struct dict_avp_data 		data = { 
					483, 					/* Code */
					0, 					/* Vendor */
					"Accounting-Realtime-Required",		/* Name */
					AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 	/* Fixed flags */
					AVP_FLAG_MANDATORY,			/* Fixed flag values */
					AVP_TYPE_INTEGER32 			/* base type of data */
					};
			/* Create the Enumerated type, and then the AVP */
			CHECK_dict_new( DICT_TYPE, &tdata , NULL, &type);
			CHECK_dict_new( DICT_ENUMVAL, &t_1 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_2 , type, NULL);
			CHECK_dict_new( DICT_ENUMVAL, &t_3 , type, NULL);
			CHECK_dict_new( DICT_AVP, &data , type, NULL);
		}
		
	}
	
	/* Commands section */
	{
		/* To avoid defining global variables for all the AVP that we use here, we do search the dictionary in each sub-block.
		 * This is far from optimal, but the code is clearer like this, and the time it requires at execution is not noticeable.
		 */
		
		/* Generic message syntax when the 'E' bit is set */
		{
			/*
				Refer RFC6733 section 7.3
			*/
			struct dict_object * cmd_error;
			struct local_rules_definition rules[] = 
						{ 	 {  "Session-Id", 			RULE_FIXED_HEAD,0, 1 }
							,{  "Origin-Host",			RULE_REQUIRED, -1, 1 }
							,{  "Origin-Realm",			RULE_REQUIRED, -1, 1 }
							,{  "Result-Code",			RULE_REQUIRED, -1, 1 }
							,{  "Origin-State-Id",			RULE_OPTIONAL, -1, 1 }
							,{  "Error-Message",			RULE_OPTIONAL, -1, 1 }
							,{  "Error-Reporting-Host",		RULE_OPTIONAL, -1, 1 }
							,{  "Failed-AVP",			RULE_OPTIONAL, -1, 1 }
							,{  "Proxy-Info",			RULE_OPTIONAL, -1,-1 }
						};
			CHECK_FCT( fd_dict_get_error_cmd(dict, &cmd_error) );
			PARSE_loc_rules( rules, cmd_error );
		}		
		
		/* Capabilities-Exchange-Request */
		{
			/*
				Refer RFC6733 section 5.3.1.
			*/
			struct dict_object * cmd;
			struct dict_cmd_data data = { 
					257, 					/* Code */
					#if CC_CAPABILITIES_EXCHANGE != 257
					#error "CC_CAPABILITIES_EXCHANGE definition mismatch"
					#endif
					"Capabilities-Exchange-Request", 	/* Name */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE | CMD_FLAG_RETRANSMIT | CMD_FLAG_ERROR, 	/* Fixed flags */
					CMD_FLAG_REQUEST 			/* Fixed flag values */
					};
			struct local_rules_definition rules[] = 
						{ 	 {  "Origin-Host", 			RULE_REQUIRED, -1, 1 }
							,{  "Origin-Realm",			RULE_REQUIRED, -1, 1 }
							,{  "Host-IP-Address",			RULE_REQUIRED, -1,-1 }
							,{  "Vendor-Id",			RULE_REQUIRED, -1, 1 }
							,{  "Product-Name",			RULE_REQUIRED, -1, 1 }
							,{  "Origin-State-Id",			RULE_OPTIONAL, -1, 1 }
							,{  "Supported-Vendor-Id",		RULE_OPTIONAL, -1,-1 }
							,{  "Auth-Application-Id",		RULE_OPTIONAL, -1,-1 }
							,{  "Inband-Security-Id",		RULE_OPTIONAL, -1,-1 }
							,{  "Acct-Application-Id",		RULE_OPTIONAL, -1,-1 }
							,{  "Vendor-Specific-Application-Id",	RULE_OPTIONAL, -1,-1 }
							,{  "Firmware-Revision",		RULE_OPTIONAL, -1, 1 }
						};
			
			CHECK_dict_new( DICT_COMMAND, &data , NULL, &cmd);
			PARSE_loc_rules( rules, cmd );
		}
		
		/* Capabilities-Exchange-Answer */
		{
			/*
				Refer RFC6733 section 5.3.2.
			*/
			struct dict_object * cmd;
			struct dict_cmd_data data = { 
					257, 					/* Code */
					#if CC_CAPABILITIES_EXCHANGE != 257
					#error "CC_CAPABILITIES_EXCHANGE definition mismatch"
					#endif
					"Capabilities-Exchange-Answer", 	/* Name */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE | CMD_FLAG_RETRANSMIT, 	/* Fixed flags */
					0 					/* Fixed flag values */
					};
			struct local_rules_definition rules[] = 
						{ 	 {  "Result-Code", 			RULE_REQUIRED, -1, 1 }
						 	,{  "Origin-Host", 			RULE_REQUIRED, -1, 1 }
							,{  "Origin-Realm",			RULE_REQUIRED, -1, 1 }
							,{  "Host-IP-Address",			RULE_REQUIRED, -1,-1 }
							,{  "Vendor-Id",			RULE_REQUIRED, -1, 1 }
							,{  "Product-Name",			RULE_REQUIRED, -1, 1 }
							,{  "Origin-State-Id",			RULE_OPTIONAL, -1, 1 }
							,{  "Error-Message",			RULE_OPTIONAL, -1, 1 }
							,{  "Failed-AVP",			RULE_OPTIONAL, -1, 1 }
							,{  "Supported-Vendor-Id",		RULE_OPTIONAL, -1,-1 }
							,{  "Auth-Application-Id",		RULE_OPTIONAL, -1,-1 }
							,{  "Inband-Security-Id",		RULE_OPTIONAL, -1,-1 }
							,{  "Acct-Application-Id",		RULE_OPTIONAL, -1,-1 }
							,{  "Vendor-Specific-Application-Id",	RULE_OPTIONAL, -1,-1 }
							,{  "Firmware-Revision",		RULE_OPTIONAL, -1, 1 }
						};
			
			CHECK_dict_new( DICT_COMMAND, &data , NULL, &cmd);
			PARSE_loc_rules( rules, cmd );
		}
		
		/* Disconnect-Peer-Request */
		{
			/*
				Refer RFC6733 section 5.4.1
			*/
			struct dict_object * cmd;
			struct dict_cmd_data data = { 
					282, 					/* Code */
					#if CC_DISCONNECT_PEER != 282
					#error "CC_DISCONNECT_PEER definition mismatch"
					#endif
					"Disconnect-Peer-Request", 		/* Name */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE | CMD_FLAG_RETRANSMIT | CMD_FLAG_ERROR, 	/* Fixed flags */
					CMD_FLAG_REQUEST 			/* Fixed flag values */
					};
			struct local_rules_definition rules[] = 
						{ 	 {  "Origin-Host", 			RULE_REQUIRED, -1, 1 }
							,{  "Origin-Realm",			RULE_REQUIRED, -1, 1 }
							,{  "Disconnect-Cause",			RULE_REQUIRED, -1, 1 }
						};
			
			CHECK_dict_new( DICT_COMMAND, &data , NULL, &cmd);
			PARSE_loc_rules( rules, cmd );
		}
		
		/* Disconnect-Peer-Answer */
		{
			/*
				Refer RFC6733 section 5.4.2
			*/
			struct dict_object * cmd;
			struct dict_cmd_data data = { 
					282, 					/* Code */
					#if CC_DISCONNECT_PEER != 282
					#error "CC_DISCONNECT_PEER definition mismatch"
					#endif
					"Disconnect-Peer-Answer", 		/* Name */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE | CMD_FLAG_RETRANSMIT, 	/* Fixed flags */
					0 					/* Fixed flag values */
					};
			struct local_rules_definition rules[] = 
						{ 	 {  "Result-Code", 			RULE_REQUIRED, -1, 1 }
						 	,{  "Origin-Host", 			RULE_REQUIRED, -1, 1 }
							,{  "Origin-Realm",			RULE_REQUIRED, -1, 1 }
							,{  "Error-Message",			RULE_OPTIONAL, -1, 1 }
							,{  "Failed-AVP",			RULE_OPTIONAL, -1, 1 }
						};
			
			CHECK_dict_new( DICT_COMMAND, &data , NULL, &cmd);
			PARSE_loc_rules( rules, cmd );
		}

		/* Device-Watchdog-Request */
		{
			/*
				Refer RFC6733 section 5.5.1
			*/
			struct dict_object * cmd;
			struct dict_cmd_data data = { 
					280, 					/* Code */
					#if CC_DEVICE_WATCHDOG != 280
					#error "CC_DEVICE_WATCHDOG definition mismatch"
					#endif
					"Device-Watchdog-Request", 		/* Name */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE | CMD_FLAG_RETRANSMIT | CMD_FLAG_ERROR, 	/* Fixed flags */
					CMD_FLAG_REQUEST 			/* Fixed flag values */
					};
			struct local_rules_definition rules[] = 
						{ 	 {  "Origin-Host", 			RULE_REQUIRED, -1, 1 }
							,{  "Origin-Realm",			RULE_REQUIRED, -1, 1 }
							,{  "Origin-State-Id",			RULE_OPTIONAL, -1, 1 }
						};
			
			CHECK_dict_new( DICT_COMMAND, &data , NULL, &cmd);
			PARSE_loc_rules( rules, cmd );
		}
		
		/* Device-Watchdog-Answer */
		{
			/*
				Refer RFC6733 section 5.5.2
			*/
			struct dict_object * cmd;
			struct dict_cmd_data data = { 
					280, 					/* Code */
					#if CC_DEVICE_WATCHDOG != 280
					#error "CC_DEVICE_WATCHDOG definition mismatch"
					#endif
					"Device-Watchdog-Answer", 		/* Name */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE | CMD_FLAG_RETRANSMIT, 	/* Fixed flags */
					0 					/* Fixed flag values */
					};
			struct local_rules_definition rules[] = 
						{ 	 {  "Result-Code", 			RULE_REQUIRED, -1, 1 }
						 	,{  "Origin-Host", 			RULE_REQUIRED, -1, 1 }
							,{  "Origin-Realm",			RULE_REQUIRED, -1, 1 }
							,{  "Error-Message",			RULE_OPTIONAL, -1, 1 }
							,{  "Failed-AVP",			RULE_OPTIONAL, -1, 1 }
							,{  "Origin-State-Id",			RULE_OPTIONAL, -1, 1 }
						};
			
			CHECK_dict_new( DICT_COMMAND, &data , NULL, &cmd);
			PARSE_loc_rules( rules, cmd );
		}
		
		/* Re-Auth-Request */
		{
			/*
				Refer RFC6733 section 8.3.1
			*/
			struct dict_object * cmd;
			struct dict_cmd_data data = { 
					258, 					/* Code */
					#if CC_RE_AUTH != 258
					#error "CC_RE_AUTH definition mismatch"
					#endif
					"Re-Auth-Request", 			/* Name */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE | CMD_FLAG_ERROR,	/* Fixed flags */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE	/* Fixed flag values */
					};
			struct local_rules_definition rules[] = 
						{ 	 {  "Session-Id", 			RULE_FIXED_HEAD, -1, 1 }
						 	,{  "Origin-Host", 			RULE_REQUIRED,   -1, 1 }
							,{  "Origin-Realm",			RULE_REQUIRED,   -1, 1 }
							,{  "Destination-Realm",		RULE_REQUIRED,   -1, 1 }
						 	,{  "Destination-Host", 		RULE_REQUIRED,   -1, 1 }
						 	,{  "Auth-Application-Id", 		RULE_REQUIRED,   -1, 1 }
						 	,{  "Re-Auth-Request-Type", 		RULE_REQUIRED,   -1, 1 }
							,{  "User-Name",			RULE_OPTIONAL,   -1, 1 }
							,{  "Origin-State-Id",			RULE_OPTIONAL,   -1, 1 }
							,{  "Proxy-Info",			RULE_OPTIONAL,   -1,-1 }
							,{  "Route-Record",			RULE_OPTIONAL,   -1,-1 }
						};
			
			CHECK_dict_new( DICT_COMMAND, &data , NULL, &cmd);
			PARSE_loc_rules( rules, cmd );
		}
		
		/* Re-Auth-Answer */
		{
			/*
				Refer RFC6733 section 8.3.2
			*/
			struct dict_object * cmd;
			struct dict_cmd_data data = { 
					258, 					/* Code */
					#if CC_RE_AUTH != 258
					#error "CC_RE_AUTH definition mismatch"
					#endif
					"Re-Auth-Answer", 			/* Name */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE,	/* Fixed flags */
							   CMD_FLAG_PROXIABLE	/* Fixed flag values */
					};
			struct local_rules_definition rules[] = 
						{ 	 {  "Session-Id", 			RULE_FIXED_HEAD, -1, 1 }
 							,{  "Result-Code", 			RULE_REQUIRED,   -1, 1 }
						 	,{  "Origin-Host", 			RULE_REQUIRED,   -1, 1 }
							,{  "Origin-Realm",			RULE_REQUIRED,   -1, 1 }
							,{  "User-Name",			RULE_OPTIONAL,   -1, 1 }
							,{  "Origin-State-Id",			RULE_OPTIONAL,   -1, 1 }
							,{  "Error-Message",			RULE_OPTIONAL,   -1, 1 }
							,{  "Error-Reporting-Host",		RULE_OPTIONAL,   -1, 1 }
							,{  "Failed-AVP",			RULE_OPTIONAL,   -1, 1 }
							,{  "Redirect-Host",			RULE_OPTIONAL,   -1,-1 }
							,{  "Redirect-Host-Usage",		RULE_OPTIONAL,   -1, 1 }
							,{  "Redirect-Max-Cache-Time",		RULE_OPTIONAL,   -1, 1 }
							,{  "Proxy-Info",			RULE_OPTIONAL,   -1,-1 }
						};
			
			CHECK_dict_new( DICT_COMMAND, &data , NULL, &cmd);
			PARSE_loc_rules( rules, cmd );
		}
		
		/* Session-Termination-Request */
		{
			/*
				Refer RFC6733 section 8.4.1
			*/
			struct dict_object * cmd;
			struct dict_cmd_data data = { 
					275, 					/* Code */
					#if CC_SESSION_TERMINATION != 275
					#error "CC_SESSION_TERMINATION definition mismatch"
					#endif
					"Session-Termination-Request", 		/* Name */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE | CMD_FLAG_ERROR,	/* Fixed flags */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE	/* Fixed flag values */
					};
			struct local_rules_definition rules[] = 
						{ 	 {  "Session-Id", 			RULE_FIXED_HEAD, -1, 1 }
						 	,{  "Origin-Host", 			RULE_REQUIRED,   -1, 1 }
							,{  "Origin-Realm",			RULE_REQUIRED,   -1, 1 }
							,{  "Destination-Realm",		RULE_REQUIRED,   -1, 1 }
						 	,{  "Auth-Application-Id", 		RULE_REQUIRED,   -1, 1 }
						 	,{  "Termination-Cause", 		RULE_REQUIRED,   -1, 1 }
							,{  "User-Name",			RULE_OPTIONAL,   -1, 1 }
							,{  "Destination-Host",			RULE_OPTIONAL,   -1, 1 }
							,{  "Class",				RULE_OPTIONAL,   -1,-1 }
							,{  "Origin-State-Id",			RULE_OPTIONAL,   -1, 1 }
							,{  "Proxy-Info",			RULE_OPTIONAL,   -1,-1 }
							,{  "Route-Record",			RULE_OPTIONAL,   -1,-1 }
						};
			
			CHECK_dict_new( DICT_COMMAND, &data , NULL, &cmd);
			PARSE_loc_rules( rules, cmd );
		}
		
		/* Session-Termination-Answer */
		{
			/*
				Refer RFC6733 section 8.4.2
			*/
			struct dict_object * cmd;
			struct dict_cmd_data data = { 
					275, 					/* Code */
					#if CC_SESSION_TERMINATION != 275
					#error "CC_SESSION_TERMINATION definition mismatch"
					#endif
					"Session-Termination-Answer", 		/* Name */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE,	/* Fixed flags */
							   CMD_FLAG_PROXIABLE	/* Fixed flag values */
					};
			struct local_rules_definition rules[] = 
						{ 	 {  "Session-Id", 			RULE_FIXED_HEAD, -1, 1 }
 							,{  "Result-Code", 			RULE_REQUIRED,   -1, 1 }
						 	,{  "Origin-Host", 			RULE_REQUIRED,   -1, 1 }
							,{  "Origin-Realm",			RULE_REQUIRED,   -1, 1 }
							,{  "User-Name",			RULE_OPTIONAL,   -1, 1 }
							,{  "Class",				RULE_OPTIONAL,   -1,-1 }
							,{  "Error-Message",			RULE_OPTIONAL,   -1, 1 }
							,{  "Error-Reporting-Host",		RULE_OPTIONAL,   -1, 1 }
							,{  "Failed-AVP",			RULE_OPTIONAL,   -1, 1 }
							,{  "Origin-State-Id",			RULE_OPTIONAL,   -1, 1 }
							,{  "Redirect-Host",			RULE_OPTIONAL,   -1,-1 }
							,{  "Redirect-Host-Usage",		RULE_OPTIONAL,   -1, 1 }
							,{  "Redirect-Max-Cache-Time",		RULE_OPTIONAL,   -1, 1 }
							,{  "Proxy-Info",			RULE_OPTIONAL,   -1,-1 }
						};
			
			CHECK_dict_new( DICT_COMMAND, &data , NULL, &cmd);
			PARSE_loc_rules( rules, cmd );
		}
		
		/* Abort-Session-Request */
		{
			/*
				Refer RFC6733 section 8.5.1
			*/
			struct dict_object * cmd;
			struct dict_cmd_data data = { 
					274, 					/* Code */
					#if CC_ABORT_SESSION != 274
					#error "CC_ABORT_SESSION definition mismatch"
					#endif
					"Abort-Session-Request", 		/* Name */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE | CMD_FLAG_ERROR,	/* Fixed flags */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE	/* Fixed flag values */
					};
			struct local_rules_definition rules[] = 
						{ 	 {  "Session-Id", 			RULE_FIXED_HEAD, -1, 1 }
						 	,{  "Origin-Host", 			RULE_REQUIRED,   -1, 1 }
							,{  "Origin-Realm",			RULE_REQUIRED,   -1, 1 }
							,{  "Destination-Realm",		RULE_REQUIRED,   -1, 1 }
							,{  "Destination-Host",			RULE_REQUIRED,   -1, 1 }
						 	,{  "Auth-Application-Id", 		RULE_REQUIRED,   -1, 1 }
							,{  "User-Name",			RULE_OPTIONAL,   -1, 1 }
							,{  "Origin-State-Id",			RULE_OPTIONAL,   -1, 1 }
							,{  "Proxy-Info",			RULE_OPTIONAL,   -1,-1 }
							,{  "Route-Record",			RULE_OPTIONAL,   -1,-1 }
						};
			
			CHECK_dict_new( DICT_COMMAND, &data , NULL, &cmd);
			PARSE_loc_rules( rules, cmd );
		}
		
		/* Abort-Session-Answer */
		{
			/*
				Refer RFC6733 section 8.5.2
			*/
			struct dict_object * cmd;
			struct dict_cmd_data data = { 
					274, 					/* Code */
					#if CC_ABORT_SESSION != 274
					#error "CC_ABORT_SESSION definition mismatch"
					#endif
					"Abort-Session-Answer", 		/* Name */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE,	/* Fixed flags */
							   CMD_FLAG_PROXIABLE	/* Fixed flag values */
					};
			struct local_rules_definition rules[] = 
						{ 	 {  "Session-Id", 			RULE_FIXED_HEAD, -1, 1 }
 							,{  "Result-Code", 			RULE_REQUIRED,   -1, 1 }
						 	,{  "Origin-Host", 			RULE_REQUIRED,   -1, 1 }
							,{  "Origin-Realm",			RULE_REQUIRED,   -1, 1 }
							,{  "User-Name",			RULE_OPTIONAL,   -1, 1 }
							,{  "Origin-State-Id",			RULE_OPTIONAL,   -1, 1 }
							,{  "Error-Message",			RULE_OPTIONAL,   -1, 1 }
							,{  "Error-Reporting-Host",		RULE_OPTIONAL,   -1, 1 }
							,{  "Failed-AVP",			RULE_OPTIONAL,   -1, 1 }
							,{  "Redirect-Host",			RULE_OPTIONAL,   -1,-1 }
							,{  "Redirect-Host-Usage",		RULE_OPTIONAL,   -1, 1 }
							,{  "Redirect-Max-Cache-Time",		RULE_OPTIONAL,   -1, 1 }
							,{  "Proxy-Info",			RULE_OPTIONAL,   -1,-1 }
						};
			
			CHECK_dict_new( DICT_COMMAND, &data , NULL, &cmd);
			PARSE_loc_rules( rules, cmd );
		}
		
		/* Accounting-Request */
		{
			/*
				Refer RFC6733 section 9.7.1
			*/
			struct dict_object * cmd;
			struct dict_cmd_data data = { 
					271, 					/* Code */
					#if CC_ACCOUNTING != 271
					#error "CC_ACCOUNTING definition mismatch"
					#endif
					"Accounting-Request", 			/* Name */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE | CMD_FLAG_ERROR,	/* Fixed flags */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE	/* Fixed flag values */
					};
			struct local_rules_definition rules[] = 
						{ 	 {  "Session-Id", 			RULE_FIXED_HEAD, -1, 1 }
						 	,{  "Origin-Host", 			RULE_REQUIRED,   -1, 1 }
							,{  "Origin-Realm",			RULE_REQUIRED,   -1, 1 }
							,{  "Destination-Realm",		RULE_REQUIRED,   -1, 1 }
							,{  "Accounting-Record-Type",		RULE_REQUIRED,   -1, 1 }
							,{  "Accounting-Record-Number",		RULE_REQUIRED,   -1, 1 }
							,{  "Acct-Application-Id",		RULE_OPTIONAL,   -1, 1 }
							,{  "Vendor-Specific-Application-Id",	RULE_OPTIONAL,   -1, 1 }
							,{  "User-Name",			RULE_OPTIONAL,   -1, 1 }
							,{  "Destination-Host",			RULE_OPTIONAL,   -1, 1 }
							,{  "Accounting-Sub-Session-Id",	RULE_OPTIONAL,   -1, 1 }
							,{  "Acct-Session-Id",			RULE_OPTIONAL,   -1, 1 }
							,{  "Acct-Multi-Session-Id",		RULE_OPTIONAL,   -1, 1 }
							,{  "Acct-Interim-Interval",		RULE_OPTIONAL,   -1, 1 }
							,{  "Accounting-Realtime-Required",	RULE_OPTIONAL,   -1, 1 }
							,{  "Origin-State-Id",			RULE_OPTIONAL,   -1, 1 }
							,{  "Event-Timestamp",			RULE_OPTIONAL,   -1, 1 }
							,{  "Proxy-Info",			RULE_OPTIONAL,   -1,-1 }
							,{  "Route-Record",			RULE_OPTIONAL,   -1,-1 }
						};
			
			CHECK_dict_new( DICT_COMMAND, &data , NULL, &cmd);
			PARSE_loc_rules( rules, cmd );
		}
		
		/* Accounting-Answer */
		{
			/*
				Refer RFC6733 section 8.7.2
			*/
			struct dict_object * cmd;
			struct dict_cmd_data data = { 
					271, 					/* Code */
					#if CC_ACCOUNTING != 271
					#error "CC_ACCOUNTING definition mismatch"
					#endif
					"Accounting-Answer", 			/* Name */
					CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE,	/* Fixed flags */
							   CMD_FLAG_PROXIABLE	/* Fixed flag values */
					};
			struct local_rules_definition rules[] = 
						{ 	 {  "Session-Id", 			RULE_FIXED_HEAD, -1, 1 }
 							,{  "Result-Code", 			RULE_REQUIRED,   -1, 1 }
						 	,{  "Origin-Host", 			RULE_REQUIRED,   -1, 1 }
							,{  "Origin-Realm",			RULE_REQUIRED,   -1, 1 }
							,{  "Accounting-Record-Type",		RULE_REQUIRED,   -1, 1 }
							,{  "Accounting-Record-Number",		RULE_REQUIRED,   -1, 1 }
							,{  "Acct-Application-Id",		RULE_OPTIONAL,   -1, 1 }
							,{  "Vendor-Specific-Application-Id",	RULE_OPTIONAL,   -1, 1 }
							,{  "User-Name",			RULE_OPTIONAL,   -1, 1 }
							,{  "Accounting-Sub-Session-Id",	RULE_OPTIONAL,   -1, 1 }
							,{  "Acct-Session-Id",			RULE_OPTIONAL,   -1, 1 }
							,{  "Acct-Multi-Session-Id",		RULE_OPTIONAL,   -1, 1 }
							,{  "Error-Message",			RULE_OPTIONAL,   -1, 1 }
							,{  "Error-Reporting-Host",		RULE_OPTIONAL,   -1, 1 }
							,{  "Failed-AVP",			RULE_OPTIONAL,   -1, 1 }
							,{  "Acct-Interim-Interval",		RULE_OPTIONAL,   -1, 1 }
							,{  "Accounting-Realtime-Required",	RULE_OPTIONAL,   -1, 1 }
							,{  "Origin-State-Id",			RULE_OPTIONAL,   -1, 1 }
							,{  "Event-Timestamp",			RULE_OPTIONAL,   -1, 1 }
							,{  "Proxy-Info",			RULE_OPTIONAL,   -1,-1 }
						};
			
			CHECK_dict_new( DICT_COMMAND, &data , NULL, &cmd);
			PARSE_loc_rules( rules, cmd );
		}
	}

	return 0;
}
