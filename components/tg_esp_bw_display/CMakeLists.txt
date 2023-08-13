idf_component_register(
SRCS
        "disp_proto_i2c.c" 
        "disp_proto.c"
        "bw_disp.c" 
        "bw_disp_sh1106.c" 
INCLUDE_DIRS 
        "include"
REQUIRES
        driver log
)