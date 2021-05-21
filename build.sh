rm -rf components/schemas
ag asyncapi.yaml https://github.com/dickeyf/esp32-asyncapi-codegen.git -o components/schemas
idf.py build