# The dict_rfc4006bis_avps extension
PROJECT("dict_rfc4006bis_avps library generated by FdGen " C)

# Compile as a module
FD_ADD_EXTENSION(dict_rfc4006bis_avps dict_rfc4006bis_avps.c)


####
## INSTALL section ##

INSTALL(TARGETS dict_rfc4006bis_avps
	LIBRARY DESTINATION ${INSTALL_EXTENSIONS_SUFFIX}
COMPONENT freeDiameter-dictionary-RFC4006BIS_AVPS)



