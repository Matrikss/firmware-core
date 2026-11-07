set(srcs
    "services/ag_client.cpp"
    "services/payload_serializer.cpp"
    "backends/wifi_http_client.cpp"

    # Binary payload encoder (vendored as-is).
    "lib/payload-encoder/src/PayloadEncoder.cpp"
)

# CoAP packet helpers are only needed by the not-yet-implemented cellular
# CoAP backend.  Keep them out of the normal Wi-Fi-only firmware builds to
# avoid compiling unreachable code.
if(CONFIG_AG_CLIENT_CELLULAR_SUPPORT)
    list(APPEND srcs
        "lib/coap-packet/src/CoapBuilder.cpp"
        "lib/coap-packet/src/CoapParser.cpp"
    )
endif()

idf_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS
        "."
        "lib/coap-packet/src"
        "lib/payload-encoder/src"
    REQUIRES
        airgradient-common
        esp_http_client
        json
)
