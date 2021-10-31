hunter_config(bcos-framework VERSION 3.0.0-local
	URL https://${URL_BASE}/cyjseagull/bcos-framework/archive/07c7cdf4379d04939d68b2b5ff5d9e734d5ebbf3.tar.gz
    SHA1 5cdde2db3645b4d6c9ac1c7a15cccffbc70fe2d5
	CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON
)

hunter_config(bcos-crypto
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-crypto/archive/255002b047b359a45c953d1dab29efd2ff6eb080.tar.gz
    SHA1 4d02de20be1f9bf79d762c5b8686368286504e07
    CMAKE_ARGS URL_BASE=${URL_BASE}
)