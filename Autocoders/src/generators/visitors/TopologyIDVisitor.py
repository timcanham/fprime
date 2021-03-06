#!/bin/env python
#===============================================================================
# NAME: TopologyIDVisitor.py
#
# DESCRIPTION: A visitor responsible for the generation of component
#              base class source code file.
#
# AUTHOR: reder
# EMAIL:  reder@jpl.nasa.gov
# DATE CREATED  : Feb 5, 2007
#
# Copyright 2013, California Institute of Technology.
# ALL RIGHTS RESERVED. U.S. Government Sponsorship acknowledged.
#===============================================================================
#
# Python standard modules
#
import logging
import os
import sys
import time
from optparse import OptionParser
#
# Python extention modules and custom interfaces
#
#from Cheetah import Template
#from utils import version
from utils import ConfigManager
from models import ModelParser
#from utils import DiffAndRename
from generators.visitors import AbstractVisitor
from generators import formatters
#
# Import precompiled templates here
#
from generators.templates.topology import publicTopologyID
#from generators.templates import finishTopologyCpp
#
# Universal globals used within module go here.
# (DO NOT USE MANY!)
#
# Global logger init. below.
PRINT = logging.getLogger('output')
DEBUG = logging.getLogger('debug')
#
# Module class or classes go here.
class TopologyIDVisitor(AbstractVisitor.AbstractVisitor):
    """
    A visitor class responsible for generation of base ID/window range export CSV files
    """
    __instance = None
    __config   = None
    __fp       = None
    __form     = None
    __form_comment = None
    __model_parser = None
    
    def __init__(self):
        """
        Constructor.
        """
        self.__config       = ConfigManager.ConfigManager.getInstance()
        self.__form         = formatters.Formatters()
        self.__form_comment = formatters.CommentFormatters()
        self.__model_parser = ModelParser.ModelParser.getInstance()
        DEBUG.info("TopologyIDVisitor: Instanced.")
        self.bodytext       = ""
        self.prototypetext  = ""

        
    def _writeTmpl(self, c, visit_str):
        """
        Wrapper to write tmpl to files desc.
        """
        DEBUG.debug('TopologyIDVisitor:%s' % visit_str)
        DEBUG.debug('===================================')
        DEBUG.debug(c)
        self.__fp.writelines(c.__str__())
        DEBUG.debug('===================================')
        
        
    def initFilesVisit(self, obj):
        """
        Defined to generate files for generated code products.
        @parms obj: the instance of the component model to visit.
        """
        # Build filename here...
        if len(obj.get_comp_list()) > 0:
            xml_file = obj.get_comp_list()[0].get_xml_filename()
            x = xml_file.split(".")
            s = self.__config.get("assembly","TopologyXML").split(".")
            l = len(s[0])
            #
            if (x[0][-l:] == s[0]) & (x[1] == s[1]):
                filename = x[0].split(s[0])[0] + self.__config.get("assembly","TopologyID")
                PRINT.info("Generating code filename: %s topology, using default XML filename prefix..." % filename)
            else:
                msg = "XML file naming format not allowed (must be XXXAppAi.xml), Filename: %s" % xml_file
                PRINT.info(msg)
                raise

            # Open file for writting here...
            DEBUG.info('Open file: %s' % filename)
            self.__fp = open(filename,'w')
            if self.__fp == None:
                raise "Could not open %s file." % filename
            DEBUG.info('Completed')
        else:
            PRINT.info('ERROR: NO COMPONENTS FOUND IN TOPOLOGY XML FILE...')
            sys.exit(-1)


    def startSourceFilesVisit(self, obj):
        """
        Defined to generate starting static code within files.
        """
        pass


    def includes1Visit(self, obj):
        """
        Defined to generate includes within a file.
        Usually used for the base classes but also for Port types
        @parms args: the instance of the concrete element to operation on.
        """
        pass


    def includes2Visit(self, obj):
        """
        Defined to generate internal includes within a file.
        Usually used for data type includes and system includes.
        @parms args: the instance of the concrete element to operation on.
        """
        pass


    def namespaceVisit(self, obj):
        """
        Defined to generate namespace code within a file.
        Also any pre-condition code is generated.
        @parms args: the instance of the concrete element to operation on.
        """
        pass


    def publicVisit(self, obj):
        """
        Defined to generate public stuff within a class.
        @parms args: the instance of the concrete element to operation on.
        """
        c = publicTopologyID.publicTopologyID()
  
        c.id_list = [] #Contents will be strings in the form 'component name,instance name,base id,window range'
        
        #
        #Generate Set Window/Base ID Method
        for id_tuple in obj.get_base_id_list():
            n = id_tuple[0]
            type = id_tuple[3].get_type()
            base_id = id_tuple[1]
            window_id = id_tuple[2]
            
            instance_list = [type, n , base_id , window_id]
            
            
            c.id_list.append(",".join(str(x) for x in instance_list))
        #
        
        self._writeTmpl(c, "publicVisit")


    def protectedVisit(self, obj):
        """
        Defined to generate protected stuff within a class.
        @parms args: the instance of the concrete element to operation on.
        """
        pass


    def privateVisit(self, obj):
        """
        Defined to generate private stuff within a class.
        @parms args: the instance of the concrete element to operation on.
        """
        pass


    def finishSourceFilesVisit(self, obj):
        """
        Defined to generate ending static code within files.
        """
        #c = finishComponentCpp.finishComponentCpp()
        #self._writeTmpl(c, "finishSourceFilesVisit")
        self.__fp.close()


if __name__ == '__main__':
    pass
