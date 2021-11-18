hunter_config(bcos-framework VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/68b587adf72d4909e0efd46d600bab0b772a9de2.tar.gz
    SHA1 a9ae41deedc68ff648b27f98bf9100f4315ab9a7
	CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON
)

hunter_config(bcos-crypto
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-crypto/archive/255002b047b359a45c953d1dab29efd2ff6eb080.tar.gz
    SHA1 4d02de20be1f9bf79d762c5b8686368286504e07
    CMAKE_ARGS URL_BASE=${URL_BASE}
)