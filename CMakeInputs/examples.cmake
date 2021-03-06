set(DIR_EXAMPLES_OUTPUT "${PROJECT_BINARY_DIR}/examples")

# Single-file examples
set(LIST_EXAMPLES
#  "tutorial-gen-new-shared-info"
#  "tutorial-app"
#  "tutorial-app-sub"
#  "app-template"
#  "ndn-lite-sd-peer"
#  "udp-basic-producer"
#  "udp-basic-consumer"
#  "udp-group-producer"
#  "udp-group-consumer"
#  "anchor"
#  "basic-node"
  "normal-node"
  "normal-node-demo1"
  "normal-node-demo2"
  "normal-node-demo3"
  "normal-node-demo4"
  "normal-node-demo5"
  "normal-node-demo6"
  "normal-node-demo7"
  "normal-node-demo8"
  "normal-node-demo9"
  "normal-node-demo10"
  "normal-node-demo11"
  "normal-node-demo12"
  "normal-node-demo13"
  "normal-node-demo14"
#  "test-name-print"
#  "packet-size-test"
#  "ancmt-interest-debug"
#  "debug-server"
#  "access-control-producer"
#  "access-control-consumer"
#  "access-control-controller"
#  "nfd-basic-consumer"
#  "nfd-basic-producer"
#  "pub-sub-peer-1"
#  "pub-sub-peer-2"
#  "deamon-producer"
#  "ndn-putchunks"
#  "file-transfer-client"
#  "file-transfer-server"
#  "bootstrap"
#  "iot-light"
#  "test-repo"
)
foreach(EXAM_NAME IN LISTS LIST_EXAMPLES)
  add_executable(${EXAM_NAME} "${DIR_EXAMPLES}/${EXAM_NAME}.c")
  target_link_libraries(${EXAM_NAME} ndn-lite)
  set_target_properties(${EXAM_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${DIR_EXAMPLES_OUTPUT})
endforeach()
unset(LIST_EXAMPLES)


# Single-file App
set(LIST_EXAMPLES
  #"smart-nightlight-hub"
  #"device-motion"
  #"device-illuminance"
)
foreach(EXAM_NAME IN LISTS LIST_EXAMPLES)
  add_executable(${EXAM_NAME} "${DIR_EXAMPLES}/smart-nightlight/${EXAM_NAME}.c")
  target_link_libraries(${EXAM_NAME} ndn-lite)
  set_target_properties(${EXAM_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${DIR_EXAMPLES_OUTPUT})
endforeach()
unset(LIST_EXAMPLES)

set(LIST_EXAMPLES
  #"lock-it-when-i-leave"
  #"device-presence"
  #"device-lock"
)
foreach(EXAM_NAME IN LISTS LIST_EXAMPLES)
  add_executable(${EXAM_NAME} "${DIR_EXAMPLES}/door-lock/${EXAM_NAME}.c")
  target_link_libraries(${EXAM_NAME} ndn-lite)
  set_target_properties(${EXAM_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${DIR_EXAMPLES_OUTPUT})
endforeach()
unset(LIST_EXAMPLES)

unset(DIR_EXAMPLES_OUTPUT)
