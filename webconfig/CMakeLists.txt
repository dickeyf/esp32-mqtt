set (npm_cmd "npm")
set (npm_args "run" "build")

add_custom_target(WEBCONFIG ALL ${npm_cmd} ${npm_args}
                  WORKING_DIRECTORY ../../webconfig)
