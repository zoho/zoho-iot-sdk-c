MACRO(READ_FILE SRC DEST)
    IF(NOT EXISTS ${SRC})
        MESSAGE(FATAL_ERROR "File not found at ${SRC}")
    ENDIF()

    FILE(COPY ${SRC} DESTINATION ${PROJECT_BINARY_DIR})
    GET_FILENAME_COMPONENT(filename ${SRC} NAME)

    IF(NOT EXISTS ${PROJECT_BINARY_DIR}/${filename})
        MESSAGE(FATAL_ERROR "Special characters/ white spaces are found in file name ${SRC}.
                remove them and try again.")
    ENDIF()

    FILE(RENAME ${PROJECT_BINARY_DIR}/${filename} ${PROJECT_BINARY_DIR}/${DEST})
    EXECUTE_PROCESS(COMMAND xxd -i ${DEST} ${PROJECT_BINARY_DIR}/temp.txt)
    FILE(READ ${PROJECT_BINARY_DIR}/temp.txt CONTENTS)
    FILE(APPEND ${PROJECT_SOURCE_DIR}/include/zclient_certificates.h "${CONTENTS}")
    FILE(REMOVE ${PROJECT_BINARY_DIR}/temp.txt)
    FILE(REMOVE ${PROJECT_BINARY_DIR}/${DEST})

ENDMACRO()
