IF(CMAKE_SYSTEM MATCHES "Solaris*")
  SET(CMAKE_SHARED_LIBRARY_C_FLAGS "-KPIC") 
  SET(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-G")
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "-R")
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG_SEP ":")
  SET(CMAKE_SHARED_LIBRARY_SONAME_C_FLAG "-h")
  SET(CMAKE_SHARED_LIBRARY_SONAME_CXX_FLAG "-h")
  SET(CMAKE_SHARED_LIBRARY_CXX_FLAGS "-KPIC") 
  SET(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "-G")
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_CXX_FLAG "-R")
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_CXX_FLAG_SEP ":")
  IF(CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_SHARED_LIBRARY_C_FLAGS "-fPIC") 
    SET(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-shared")
    SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "-Wl,-R")
    SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG_SEP ":")  
    SET(CMAKE_SHARED_LIBRARY_SONAME_C_FLAG "-Wl,-h")
  ELSE(CMAKE_COMPILER_IS_GNUCC)
    SET (CMAKE_C_FLAGS_INIT "")
    SET (CMAKE_C_FLAGS_DEBUG_INIT "-g")
    SET (CMAKE_C_FLAGS_MINSIZEREL_INIT "-xO3 -DNDEBUG")
    SET (CMAKE_C_FLAGS_RELEASE_INIT "-xO2 -DNDEBUG")
    SET (CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "-xO2")
  ENDIF(CMAKE_COMPILER_IS_GNUCC)
  IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_SHARED_LIBRARY_CXX_FLAGS "-fPIC") 
    SET(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "-shared")
    SET(CMAKE_SHARED_LIBRARY_RUNTIME_CXX_FLAG "-Wl,-R")
    SET(CMAKE_SHARED_LIBRARY_RUNTIME_CXX_FLAG_SEP ":")  
    SET(CMAKE_SHARED_LIBRARY_SONAME_CXX_FLAG "-Wl,-h")
  ELSE(CMAKE_COMPILER_IS_GNUCXX)
    SET (CMAKE_CXX_FLAGS_INIT "")
    SET (CMAKE_CXX_FLAGS_DEBUG_INIT "-g")
    SET (CMAKE_CXX_FLAGS_MINSIZEREL_INIT "-xO3 -DNDEBUG")
    SET (CMAKE_CXX_FLAGS_RELEASE_INIT "-xO2 -DNDEBUG")
    SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "-xO2")
  ENDIF(CMAKE_COMPILER_IS_GNUCXX)
ENDIF(CMAKE_SYSTEM MATCHES "Solaris*")

IF(CMAKE_SYSTEM MATCHES "SunOS-4.*")
   SET(CMAKE_SHARED_LIBRARY_C_FLAGS "-PIC") 
   SET(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-shared -Wl,-r") 
   SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "-Wl,-R")
   SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG_SEP ":")  
   SET(CMAKE_SHARED_LIBRARY_SONAME_C_FLAG "-h")
   SET(CMAKE_SHARED_LIBRARY_SONAME_CXX_FLAG "-h")
ENDIF(CMAKE_SYSTEM MATCHES "SunOS-4.*")

IF(CMAKE_SYSTEM MATCHES "SunOS-5.*")
  SET(CMAKE_SHARED_LIBRARY_C_FLAGS "-KPIC") 
  SET(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-G")
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "-R")
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG_SEP ":")
  SET(CMAKE_SHARED_LIBRARY_SONAME_C_FLAG "-h")
  SET(CMAKE_SHARED_LIBRARY_SONAME_CXX_FLAG "-h")
  SET(CMAKE_SHARED_LIBRARY_CXX_FLAGS "-KPIC") 
  SET(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "-G")
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_CXX_FLAG "-R")
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_CXX_FLAG_SEP ":")
  IF(CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_SHARED_LIBRARY_C_FLAGS "-fPIC") 
    SET(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-shared")
    SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "-Wl,-R")
    SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG_SEP ":")  
    SET(CMAKE_SHARED_LIBRARY_SONAME_C_FLAG "-Wl,-h")
  ELSE(CMAKE_COMPILER_IS_GNUCC)
    SET (CMAKE_C_FLAGS_INIT "")
    SET (CMAKE_C_FLAGS_DEBUG_INIT "-g")
    SET (CMAKE_C_FLAGS_MINSIZEREL_INIT "-xO3 -DNDEBUG")
    SET (CMAKE_C_FLAGS_RELEASE_INIT "-xO2 -DNDEBUG")
    SET (CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "-xO2")
  ENDIF(CMAKE_COMPILER_IS_GNUCC)
  IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_SHARED_LIBRARY_CXX_FLAGS "-fPIC") 
    SET(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "-shared")
    SET(CMAKE_SHARED_LIBRARY_RUNTIME_CXX_FLAG "-Wl,-R")
    SET(CMAKE_SHARED_LIBRARY_RUNTIME_CXX_FLAG_SEP ":")  
    SET(CMAKE_SHARED_LIBRARY_SONAME_CXX_FLAG "-Wl,-h")
  ELSE(CMAKE_COMPILER_IS_GNUCXX)
    SET (CMAKE_CXX_FLAGS_INIT "")
    SET (CMAKE_CXX_FLAGS_DEBUG_INIT "-g")
    SET (CMAKE_CXX_FLAGS_MINSIZEREL_INIT "-xO3 -DNDEBUG")
    SET (CMAKE_CXX_FLAGS_RELEASE_INIT "-xO2 -DNDEBUG")
    SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "-xO2")
  ENDIF(CMAKE_COMPILER_IS_GNUCXX)
ENDIF(CMAKE_SYSTEM MATCHES "SunOS-5.*")

IF(CMAKE_COMPILER_IS_GNUCXX)
  IF(CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_CXX_CREATE_SHARED_LIBRARY
        "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS>  <CMAKE_SHARED_LIBRARY_SONAME_CXX_FLAG><TARGET_SONAME> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
  ELSE(CMAKE_COMPILER_IS_GNUCC)
    # Take default rule from CMakeDefaultMakeRuleVariables.cmake.
  ENDIF(CMAKE_COMPILER_IS_GNUCC)
ELSE(CMAKE_COMPILER_IS_GNUCXX)
  IF(CMAKE_CXX_COMPILER)
     SET(CMAKE_CXX_CREATE_STATIC_LIBRARY
      "<CMAKE_CXX_COMPILER> -xar -o <TARGET> <OBJECTS> "
      "<CMAKE_RANLIB> <TARGET> ")
  ENDIF(CMAKE_CXX_COMPILER)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)
INCLUDE(Platform/UnixPaths)

# Add the compiler's implicit link directories.
IF("${CMAKE_C_COMPILER_ID} ${CMAKE_CXX_COMPILER_ID}" MATCHES SunPro)
  LIST(APPEND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
    /opt/SUNWspro/lib /opt/SUNWspro/prod/lib /usr/ccs/lib)
ENDIF("${CMAKE_C_COMPILER_ID} ${CMAKE_CXX_COMPILER_ID}" MATCHES SunPro)

IF(NOT CMAKE_COMPILER_IS_GNUCC)
  SET (CMAKE_C_CREATE_PREPROCESSED_SOURCE "<CMAKE_C_COMPILER> <DEFINES> <FLAGS> -E <SOURCE> > <PREPROCESSED_SOURCE>")
  SET (CMAKE_C_CREATE_ASSEMBLY_SOURCE "<CMAKE_C_COMPILER> <DEFINES> <FLAGS> -S <SOURCE> -o <ASSEMBLY_SOURCE>")
ENDIF(NOT CMAKE_COMPILER_IS_GNUCC)

IF(NOT CMAKE_COMPILER_IS_GNUCXX)
  SET (CMAKE_CXX_CREATE_PREPROCESSED_SOURCE "<CMAKE_CXX_COMPILER> <DEFINES> <FLAGS> -E <SOURCE> > <PREPROCESSED_SOURCE>")
  SET (CMAKE_CXX_CREATE_ASSEMBLY_SOURCE "<CMAKE_CXX_COMPILER> <FLAGS> -S <SOURCE> -o <ASSEMBLY_SOURCE>")
ENDIF(NOT CMAKE_COMPILER_IS_GNUCXX)

# Initialize C link type selection flags.  These flags are used when
# building a shared library, shared module, or executable that links
# to other libraries to select whether to use the static or shared
# versions of the libraries.
IF(CMAKE_COMPILER_IS_GNUCC)
  FOREACH(type SHARED_LIBRARY SHARED_MODULE EXE)
    SET(CMAKE_${type}_LINK_STATIC_C_FLAGS "-Wl,-Bstatic")
    SET(CMAKE_${type}_LINK_DYNAMIC_C_FLAGS "-Wl,-Bdynamic")
  ENDFOREACH(type)
ELSE(CMAKE_COMPILER_IS_GNUCC)
  FOREACH(type SHARED_LIBRARY SHARED_MODULE EXE)
    SET(CMAKE_${type}_LINK_STATIC_C_FLAGS "-Bstatic")
    SET(CMAKE_${type}_LINK_DYNAMIC_C_FLAGS "-Bdynamic")
  ENDFOREACH(type)
ENDIF(CMAKE_COMPILER_IS_GNUCC)
IF(CMAKE_COMPILER_IS_GNUCXX)
  FOREACH(type SHARED_LIBRARY SHARED_MODULE EXE)
    SET(CMAKE_${type}_LINK_STATIC_CXX_FLAGS "-Wl,-Bstatic")
    SET(CMAKE_${type}_LINK_DYNAMIC_CXX_FLAGS "-Wl,-Bdynamic")
  ENDFOREACH(type)
ELSE(CMAKE_COMPILER_IS_GNUCXX)
  FOREACH(type SHARED_LIBRARY SHARED_MODULE EXE)
    SET(CMAKE_${type}_LINK_STATIC_CXX_FLAGS "-Bstatic")
    SET(CMAKE_${type}_LINK_DYNAMIC_CXX_FLAGS "-Bdynamic")
  ENDFOREACH(type)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

# The Sun linker needs to find transitive shared library dependencies
# in the -L path.
SET(CMAKE_LINK_DEPENDENT_LIBRARY_DIRS 1)

# Shared libraries with no builtin soname may not be linked safely by
# specifying the file path.
SET(CMAKE_PLATFORM_USES_PATH_WHEN_NO_SONAME 1)
