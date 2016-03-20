# Locate the iod library
# define IOD_INCLUDE_DIR

FIND_PATH(IOD_INCLUDE_DIR iod/sio.hh
			   $ENV{HOME}/local/include
			   /usr/include
			   /usr/local/include
			   /sw/include
			   /opt/local/include
			   DOC "Iod include dir")
