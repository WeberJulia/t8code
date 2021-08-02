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
  
#include <t8_schemes/t8_new_feature/t8_subelements_cxx.hxx>
#include <example/common/t8_example_common.h>
  
/* In this example, a single quad element is refined into a transition cell of a specific type. 
 * At the moment, subelements are only implemented for the quad scheme. 
 * Valid types range from 1 to 15. */ 

t8_refine_quad_to_subelements () 
{
  
  
    ("In this function we will construct a single element and refine it using subelements.\n");
  
  
  
  
    num_subelements, num_vertices;
  
    /* Chose a type between 1 and 15 */ 
  int                 type = 15;
  
    /* At the moment, subelements are only implemented for the wuad scheme. */ 
    eclass = T8_ECLASS_QUAD;
  
  
    /* Allocate memory for a quad element and initialize it */ 
    class_scheme->t8_element_new (1, &element);
  
  
  
    /* Allocate enough memory for subelements of the given type and initialize them */ 
    num_subelements =
    
  
    T8_ALLOC (t8_element_t *, num_subelements);
  
  
    /* Create all subelements for the given type from the initial quad element. */ 
    class_scheme->t8_element_to_subelement (element, type,
                                            element_subelements);
  
  
    ("The transition cell of type %i consists of %i subelements with ids ranging from 0 to %i.\n",
     type, num_subelements, num_subelements - 1);
  
  
    /* Iterate through all subelements and determine their vertex coordinates */ 
    for (subelement_id = 0; subelement_id < num_subelements; ++subelement_id) {
    
      /* determine the shape of the subelement and use it to determine the number of vertices it has (triangle -> 3 vertices) */ 
    const t8_element_shape_t shape =
      class_scheme->t8_element_shape (element_subelements[subelement_id]);
    
    
      /* Iterate over all vertices of the subelement and, determine their coordinates and print them */ 
      for (vertex_id = 0; vertex_id < num_vertices; ++vertex_id) {
      
                                               [subelement_id], 
                                               
      
                         subelement_id, vertex_id, coords[0], coords[1]);
    
  
  
    /* TODO: Print the transition cell in Paraview. */ 
    
    /* free memory */ 
    class_scheme->t8_element_destroy (1, &element);
  
  
  



main (int argc, char **argv) 
{
  
  
  
  
  
  
  
  
  
  


