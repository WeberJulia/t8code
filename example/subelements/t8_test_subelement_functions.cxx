/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element types in parallel.

  Copyright (C) 2015 the developers

  t8code is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  t8code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with t8code; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/  
  
#include <sc_refcount.h>
#include <sc_shmem.h>
#include <p4est_connectivity.h>
#include <p8est_connectivity.h>
#include <t8_schemes/t8_new_feature/t8_subelements_cxx.hxx>
#include <t8_forest/t8_forest_adapt.h>
#include <t8_forest.h>
#include <t8_cmesh_vtk.h>
#include <t8_vec.h>
#include <example/common/t8_example_common.h>
  
/* In this example, a single quad element is refined into a transition cell of a specific type. 
 * At the moment, subelements are only implemented for the quad scheme. 
 * Valid types range from 1 to 15. */ 

t8_refine_quad_to_subelements () 
{
  
  
  
  
  
    
  
    /* Chose a type between 1 and 15 */ 
  int                 type = 15;
  
    /* At the moment, subelements are only implemented for the wuad scheme. */ 
    eclass = T8_ECLASS_QUAD;
  
  
    /* Allocate memory for a quad element and initialize it */ 
    class_scheme->t8_element_new (1, &element);
  
  
  
    /* Allocate memory for subelements of the given type and initialize them */ 
    num_subelements =
    
  
  
  
    /* Create all subelements for the given type from the initial quad element. */ 
    class_scheme->t8_element_to_subelement (element, type,
                                            element_subelements);
  
    ("The transition cell of type %i consists of %i subelements.\n", type,
     
  
    /* Iterate through all subelements and determine their vertex coordinates */ 
    for (subelement_id = 0; subelement_id < num_subelements; ++subelement_id) {
    
      /* determine the shape of the subelement and use it to determine the number of vertices it has (triangle -> 3 vertices) */ 
    const t8_element_shape_t shape =
      
    
    
      /* Iterate over all vertices of the subelement and, determine their coordinates and print them */ 
      for (vertex_id = 0; vertex_id < num_vertices; ++vertex_id) {
      
                                                   [subelement_id],
                                                   
      
        ("Sub_id = %i; Vertex = %i; Coordinates = (%i,%i)\n",
         
    
  
  
    /* TODO: Print the transition cell in Paraview. */ 
    
    /* free memory */ 
    class_scheme->t8_element_destroy (1, &element);
  
  



main (int argc, char **argv) 
{
  
  
  
  
  
  
  
  
  
  


