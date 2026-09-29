idf_component_register(
    SRCS "dht11.cpp" "app_main.cpp" "wifi.cpp" "mqtt.cpp"
    INCLUDE_DIRS "."
    EMBED_TXTFILES "../certs/ca.crt"
    )
