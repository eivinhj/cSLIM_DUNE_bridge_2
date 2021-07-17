cmake_minimum_required(VERSION 2.8.9)

find_package(PahoMqttCpp REQUIRED)

dune_add_lib(paho-mqttpp3)
dune_add_lib(paho-mqtt3as)