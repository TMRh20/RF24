# get lib info from the maintained library.properties required by the Arduino IDE
file(STRINGS ${CMAKE_SOURCE_DIR}/library.properties LibInfo)
foreach(line ${LibInfo})
    string(FIND ${line} "=" label_delimiter)
    if(${label_delimiter} GREATER 0)
        math(EXPR label_delimiter "${label_delimiter} + 1")
        string(FIND ${line} "version" has_version)
        string(FIND ${line} "name" has_name)
        string(FIND ${line} "maintainer" has_maintainer)
        string(FIND ${line} "sentence" has_sentence)
        string(FIND ${line} "url" has_url)
        string(FIND ${line} "paragraph" has_paragraph)
        if(${has_version} EQUAL 0)
            string(SUBSTRING ${line} ${label_delimiter} "-1" VERSION)
        elseif(${has_name} EQUAL 0)
            string(SUBSTRING ${line} ${label_delimiter} "-1" LibName)
        elseif(${has_maintainer} EQUAL 0)
            string(SUBSTRING ${line} ${label_delimiter} "-1" CPACK_PACKAGE_CONTACT)
        elseif(${has_sentence} EQUAL 0)
            string(SUBSTRING ${line} ${label_delimiter} "-1" CPACK_PACKAGE_DESCRIPTION_SUMMARY)
        elseif(${has_url} EQUAL 0)
            string(SUBSTRING ${line} ${label_delimiter} "-1" CMAKE_PROJECT_HOMEPAGE_URL)
        elseif(${has_paragraph} EQUAL 0)
            string(SUBSTRING ${line} ${label_delimiter} "-1" CPACK_PACKAGE_DESCRIPTION)
        endif()
    endif()
endforeach()

# convert the LibName to lower case & strip surrounding whitespace
string(TOLOWER ${LibName} LibTargetName)

#parse the version information into pieces.
string(REGEX REPLACE "^([0-9]+)\\..*" "\\1" VERSION_MAJOR "${VERSION}")
string(REGEX REPLACE "^[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${VERSION}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH "${VERSION}")

# this is the library version
set(${LibName}_VERSION_MAJOR ${VERSION_MAJOR})
set(${LibName}_VERSION_MINOR ${VERSION_MINOR})
set(${LibName}_VERSION_PATCH ${VERSION_PATCH})
set(${LibName}_VERSION_STRING ${${LibName}_VERSION_MAJOR}.${${LibName}_VERSION_MINOR}.${${LibName}_VERSION_PATCH})

message(STATUS "${LibName} library version: ${${LibName}_VERSION_STRING}")
