PROJECT("freeDiameter extensions" C)

# We want all resulting objects in the same folder
SET(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

# Use the macro FD_ADD_EXTENSION(name files...) to create an extension
# It is equivalent to add_library with the appropriate parameters
# and naming conventions (.fdx : FreeDiameter eXtension)
MACRO(FD_ADD_EXTENSION EXTNAME)
  ADD_LIBRARY(${EXTNAME} MODULE ${ARGN})
  SET_TARGET_PROPERTIES(${EXTNAME} PROPERTIES PREFIX "" )
  SET_TARGET_PROPERTIES(${EXTNAME} PROPERTIES SUFFIX ".fdx" )
ENDMACRO(FD_ADD_EXTENSION)

# Use the macro FD_EXTENSION_SUBDIR(extmacroname subdir descr default) to 
# add a new extension subdirectory.
MACRO(FD_EXTENSION_SUBDIR EXTSUBDIR EXTDESCR EXTDEFAULT)
   STRING(TOUPPER ${EXTSUBDIR} EXTMACRONAME)
   IF (NOT ALL_EXTENSIONS)
     OPTION(BUILD_${EXTMACRONAME} "Build ${EXTSUBDIR}.fdx? (${EXTDESCR})" ${EXTDEFAULT})
   ENDIF (NOT ALL_EXTENSIONS)
   IF (BUILD_${EXTMACRONAME} OR ALL_EXTENSIONS)
      SUBDIRS(${EXTSUBDIR})
   ENDIF (BUILD_${EXTMACRONAME} OR ALL_EXTENSIONS)
ENDMACRO(FD_EXTENSION_SUBDIR)

OPTION(ALL_EXTENSIONS "Build ALL available extensions? (disable to select individual components)" OFF)

# The extensions include the headers of freeDiameter that contains gnutls objects
INCLUDE_DIRECTORIES(${LFDCORE_INCLUDES})


###########################
# Extensions section

####
# Diameter applications dictionaries

FD_EXTENSION_SUBDIR(dict_nasreq "NASREQ (RFC4005) Dictionary definitions"       ON)
FD_EXTENSION_SUBDIR(dict_eap    "Diameter EAP (RFC4072) Dictionary definitions" ON)

FD_EXTENSION_SUBDIR(dict_dcca   "Diameter CC (RFC4006) Dictionary definitions [incomplete]" ON)
FD_EXTENSION_SUBDIR(dict_dcca_3gpp   "Diameter CC 3GPP Dictionary definitions [incomplete]" ON)
FD_EXTENSION_SUBDIR(dict_dcca_starent   "Diameter CC Starent Dictionary definitions" ON)

FD_EXTENSION_SUBDIR(dict_sip    "Diameter SIP (RFC4740) Dictionary definitions" ON)

FD_EXTENSION_SUBDIR(dict_mip6a     "Diameter Mobile IPv6 Auth Dictionary definitions"         ON)
FD_EXTENSION_SUBDIR(dict_mip6i     "Diameter Mobile IPv6 IKE Dictionary definitions"          ON)
FD_EXTENSION_SUBDIR(dict_nas_mipv6 "Diameter NAS-to-HAAA Interaction Dictionary definitions"  ON)

FD_EXTENSION_SUBDIR(dict_legacy_xml "Load Diameter dictionary definitions from XML files."    OFF)


####
# Diameter applications

FD_EXTENSION_SUBDIR(app_acct    "Simple accounting application that stores accounting records in flat database" OFF)
FD_EXTENSION_SUBDIR(app_diameap "Diameter EAP Application server (RFC 4072)" 					OFF)
FD_EXTENSION_SUBDIR(app_radgw   "RADIUS/Diameter gateway translation - RADIUS client to Diameter server" 	OFF)
FD_EXTENSION_SUBDIR(app_sip     "Diameter SIP Authentication and Authorization server (RFC 4740)" 		OFF)

FD_EXTENSION_SUBDIR(app_redirect "Diameter Redirect server: send configurable Redirect indications to other peers" OFF)


####
# Routing extensions

FD_EXTENSION_SUBDIR(rt_busypeers "Handling of Diameter TOO_BUSY messages and relay timeouts"	ON)
FD_EXTENSION_SUBDIR(rt_default   "Configurable routing rules for freeDiameter" 		     	ON)
FD_EXTENSION_SUBDIR(rt_ereg      "Configurable routing based on regexp matching of AVP values" OFF)
FD_EXTENSION_SUBDIR(rt_ignore_dh "Stow Destination-Host in Proxy-Info, restore to Origin-Host for answers"	ON)
FD_EXTENSION_SUBDIR(rt_load_balance "Balance load over multiple equal hosts, based on outstanding requests"	ON)
FD_EXTENSION_SUBDIR(rt_randomize "Randomly choose one of the highest scored hosts and increase its score by one"	ON)
FD_EXTENSION_SUBDIR(rt_redirect  "Handling of Diameter Redirect messages" 			ON)


####
# Peers security extensions

FD_EXTENSION_SUBDIR(acl_wl "White-list based authorization of incoming connections" ON)


####
# Debug & test extensions

FD_EXTENSION_SUBDIR(dbg_monitor "Outputs periodical status information"              ON)
FD_EXTENSION_SUBDIR(dbg_msg_timings "Show some timing information for messages"      ON)
FD_EXTENSION_SUBDIR(dbg_msg_dumps "Show human-readable content of the received & sent messages"      ON)
FD_EXTENSION_SUBDIR(dbg_rt      "Routing extension for debugging the routing module" ON)
FD_EXTENSION_SUBDIR(test_app    "Testing application to send dummy message to another peer, like a Diameter 'ping'" OFF)
FD_EXTENSION_SUBDIR(test_sip    "Testing application to simulate Diameter-SIP client (RFC4740)" OFF)
FD_EXTENSION_SUBDIR(dbg_interactive "Python-interpreter based module"                OFF)
FD_EXTENSION_SUBDIR(test_netemul "Simple Diameter network emulator proxy extension (latency, PDV, duplicates)" OFF)



# The following extension have very little use except for specific tests, so we disable them except in Debug configurations.
IF (CMAKE_BUILD_TYPE MATCHES "Debug")
	FD_EXTENSION_SUBDIR(_sample     "Simple extension to demonstrate extension mechanism, for developpers" OFF)
	FD_EXTENSION_SUBDIR(test_acct   "Receive Accounting-Requests and display the data, but no storage" OFF)
	FD_EXTENSION_SUBDIR(test_rt_any "Routing extension randomly sending message to any peer available, for testing purpose" OFF)
ENDIF (CMAKE_BUILD_TYPE MATCHES "Debug")


FD_EXTENSION_SUBDIR(dict_3gpp2_avps "dict_3gpp2_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_CreditControl "dict_CreditControl extension library" ON)
FD_EXTENSION_SUBDIR(dict_CxDx "dict_CxDx extension library" ON)
FD_EXTENSION_SUBDIR(dict_Gx "dict_Gx extension library" ON)
FD_EXTENSION_SUBDIR(dict_NAS "dict_NAS extension library" ON)
FD_EXTENSION_SUBDIR(dict_Rf "dict_Rf extension library" ON)
FD_EXTENSION_SUBDIR(dict_Ro "dict_Ro extension library" ON)
FD_EXTENSION_SUBDIR(dict_Rx "dict_Rx extension library" ON)
FD_EXTENSION_SUBDIR(dict_S6as6d "dict_S6as6d extension library" ON)
FD_EXTENSION_SUBDIR(dict_S6mS6n "dict_S6mS6n extension library" ON)
FD_EXTENSION_SUBDIR(dict_S6t "dict_S6t extension library" ON)
FD_EXTENSION_SUBDIR(dict_SGd "dict_SGd extension library" ON)
FD_EXTENSION_SUBDIR(dict_SLh "dict_SLh extension library" ON)
FD_EXTENSION_SUBDIR(dict_Sd "dict_Sd extension library" ON)
FD_EXTENSION_SUBDIR(dict_Sh "dict_Sh extension library" ON)
FD_EXTENSION_SUBDIR(dict_St "dict_St extension library" ON)
FD_EXTENSION_SUBDIR(dict_T4 "dict_T4 extension library" ON)
FD_EXTENSION_SUBDIR(dict_T6aT6bT7 "dict_T6aT6bT7 extension library" ON)
FD_EXTENSION_SUBDIR(dict_Tsp "dict_Tsp extension library" ON)
#FD_EXTENSION_SUBDIR(dict_base_rfc6733 "dict_base_rfc6733 extension library" ON)
FD_EXTENSION_SUBDIR(dict_draftload_avps "dict_draftload_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_etsi283034_avps "dict_etsi283034_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_rfc4004_avps "dict_rfc4004_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_rfc4006bis_avps "dict_rfc4006bis_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_rfc4072_avps "dict_rfc4072_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_rfc4590_avps "dict_rfc4590_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_rfc5447_avps "dict_rfc5447_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_rfc5580_avps "dict_rfc5580_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_rfc5777_avps "dict_rfc5777_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_rfc5778_avps "dict_rfc5778_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_rfc6734_avps "dict_rfc6734_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_rfc6942_avps "dict_rfc6942_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_rfc7155_avps "dict_rfc7155_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_rfc7683_avps "dict_rfc7683_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_rfc7944_avps "dict_rfc7944_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29061_avps "dict_ts29061_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29128_avps "dict_ts29128_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29154_avps "dict_ts29154_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29173_avps "dict_ts29173_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29212_avps "dict_ts29212_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29214_avps "dict_ts29214_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29215_avps "dict_ts29215_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29217_avps "dict_ts29217_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29229_avps "dict_ts29229_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29272_avps "dict_ts29272_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29273_avps "dict_ts29273_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29329_avps "dict_ts29329_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29336_avps "dict_ts29336_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29337_avps "dict_ts29337_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29338_avps "dict_ts29338_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29343_avps "dict_ts29343_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29344_avps "dict_ts29344_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29345_avps "dict_ts29345_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29368_avps "dict_ts29368_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts29468_avps "dict_ts29468_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_ts32299_avps "dict_ts32299_avps extension library" ON)
FD_EXTENSION_SUBDIR(dict_S6c "dict_S6c extension library" ON)
FD_EXTENSION_SUBDIR(dict_S9 "dict_S9 extension library" ON)
