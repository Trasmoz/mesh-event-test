idf_component_register(SRCS "mesh_admin_cmd.c"
                            "buffer_mesh_msg.c"
                            "mesh_main.c"
                    INCLUDE_DIRS "." "include")
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
