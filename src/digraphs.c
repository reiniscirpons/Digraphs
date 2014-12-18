/***************************************************************************
**
*A  digraphs.c                  GAP package digraphs          Julius Jonusas
**                                                            James Mitchell 
**                                                            Michael Torpey
**                                                            Wilfred Wilson
**
**  Copyright (C) 2014 - Julius Jonusas, James Mitchell, Michael Torpey, 
**  Wilfred Wilson 
**  This file is free software, see license information at the end.
**  
*/

#include "src/digraphs.h"

#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_URL
#undef PACKAGE_VERSION

static Obj FuncDIGRAPH_OUT_NBS(Obj self, Obj digraph, Obj source, Obj range);
static Obj FuncDIGRAPH_IN_NBS(Obj self, Obj digraph);
static Obj FuncDIGRAPH_SOURCE_RANGE(Obj self, Obj digraph);

/*************************************************************************/

Int DigraphNrVertices(Obj digraph) {
  if (IsbPRec(digraph, RNamName("nrvertices"))) {
    return INT_INTOBJ(ElmPRec(digraph, RNamName("nrvertices")));
  }
  // The record comp should always be set so this should never be triggered
  ErrorQuit(
  "Digraphs: DigraphNrVertices (C):\nrec comp <nrvertices> is not set,",
  0L, 0L);
  return 0;
}

Int DigraphNrEdges(Obj digraph) {
  Obj   adj;
  Int  nr, i, n;
  if (IsbPRec(digraph, RNamName("nredges"))) {
    return INT_INTOBJ(ElmPRec(digraph, RNamName("nredges")));
  }
  n   = DigraphNrVertices(digraph);
  adj = OutNeighbours(digraph); 
  nr  = 0;
  for (i = 1; i <= n; i++) {
    nr += LEN_PLIST(ELM_PLIST(adj, i));
  }
  AssPRec(digraph, RNamName("nredges"), INTOBJ_INT(nr));
  return nr;
}

static Obj FuncDIGRAPH_NREDGES(Obj self, Obj digraph) {
  return INTOBJ_INT(DigraphNrEdges(digraph));
}

// Need to decide what we're doing with all this
Obj OutNeighbours(Obj digraph) {
  Obj adj;

  if (IsbPRec(digraph, RNamName("adj"))) {
    return ElmPRec(digraph, RNamName("adj"));
  } else if (IsbPRec(digraph, RNamName("source"))
          && IsbPRec(digraph, RNamName("range"))) {

    adj = FuncDIGRAPH_OUT_NBS( NULL,
                               ElmPRec(digraph, RNamName("nrvertices")),
                               ElmPRec(digraph, RNamName("source")),
                               ElmPRec(digraph, RNamName("range")) );
    AssPRec(digraph, RNamName("adj"), adj);
    return adj;
  }
  ErrorQuit("Digraphs: OutNeighbours (C): not enough record components set,", 0L, 0L);
  return False;
}

/****************************************************************************
**
*F  FuncGABOW_SCC
**
** `digraph' should be a list whose entries and the lists of out-neighbours
** of the vertices. So [[2,3],[1],[2]] represents the graph whose edges are
** 1->2, 1->3, 2->1 and 3->2.
**
** returns a newly constructed record with two components 'comps' and 'id' the
** elements of 'comps' are lists representing the strongly connected components
** of the directed graph, and in the component 'id' the following holds:
** id[i]=PositionProperty(comps, x-> i in x);
** i.e. 'id[i]' is the index in 'comps' of the component containing 'i'.  
** Neither the components, nor their elements are in any particular order.
**
** The algorithm is that of Gabow, based on the implementation in Sedgwick:
**   http://algs4.cs.princeton.edu/42directed/GabowSCC.java.html
** (made non-recursive to avoid problems with stack limits) and 
** the implementation of STRONGLY_CONNECTED_COMPONENTS_DIGRAPH in listfunc.c.
*/

static Obj FuncGABOW_SCC(Obj self, Obj digraph) {
  UInt end1, end2, count, level, w, v, n, nr, idw, *frames, *stack2;
  Obj  id, stack1, out, comp, comps, adj; 
 
  PLAIN_LIST(digraph);
  n = LEN_PLIST(digraph);
  if (n == 0){
    out = NEW_PREC(2);
    AssPRec(out, RNamName("id"), NEW_PLIST(T_PLIST_EMPTY+IMMUTABLE,0));
    AssPRec(out, RNamName("comps"), NEW_PLIST(T_PLIST_EMPTY+IMMUTABLE,0));
    CHANGED_BAG(out);
    return out;
  }

  end1 = 0; 
  stack1 = NEW_PLIST(T_PLIST_CYC, n); 
  //stack1 is a plist so we can use memcopy below
  SET_LEN_PLIST(stack1, n);
  
  id = NEW_PLIST(T_PLIST_CYC+IMMUTABLE, n);
  SET_LEN_PLIST(id, n);
  
  //init id
  for(v=1;v<=n;v++){
    SET_ELM_PLIST(id, v, INTOBJ_INT(0));
  }
  
  count = n;
  
  comps = NEW_PLIST(T_PLIST_TAB+IMMUTABLE, n);
  SET_LEN_PLIST(comps, 0);
  
  stack2 = malloc( (4 * n + 2) * sizeof(UInt) );
  frames = stack2 + n + 1;
  end2 = 0;
  
  for (v = 1; v <= n; v++) {
    if(INT_INTOBJ(ELM_PLIST(id, v)) == 0){
      adj =  ELM_PLIST(digraph, v);
      PLAIN_LIST(adj);
      level = 1;
      frames[0] = v; // vertex
      frames[1] = 1; // index
      frames[2] = (UInt)adj; 
      SET_ELM_PLIST(stack1, ++end1, INTOBJ_INT(v));
      stack2[++end2] = end1;
      SET_ELM_PLIST(id, v, INTOBJ_INT(end1)); 
      
      while (1) {
        if (frames[1] > (UInt) LEN_PLIST(frames[2])) {
          if (stack2[end2] == (UInt) INT_INTOBJ(ELM_PLIST(id, frames[0]))) {
            end2--;
            count++;
            nr = 0;
            do{
              nr++;
              w = INT_INTOBJ(ELM_PLIST(stack1, end1--));
              SET_ELM_PLIST(id, w, INTOBJ_INT(count));
            }while (w != frames[0]);
            
            comp = NEW_PLIST(T_PLIST_CYC+IMMUTABLE, nr);
            SET_LEN_PLIST(comp, nr);
           
            memcpy( (void *)((char *)(ADDR_OBJ(comp)) + sizeof(Obj)), 
                    (void *)((char *)(ADDR_OBJ(stack1)) + (end1 + 1) * sizeof(Obj)), 
                    (size_t)(nr * sizeof(Obj)));

            nr = LEN_PLIST(comps) + 1;
            SET_ELM_PLIST(comps, nr, comp);
            SET_LEN_PLIST(comps, nr);
            CHANGED_BAG(comps);
          }
          level--;
          if (level == 0) {
            break;
          }
          frames -= 3;
        } else {
          
          w = INT_INTOBJ(ELM_PLIST(frames[2], frames[1]++));
          idw = INT_INTOBJ(ELM_PLIST(id, w));
          
          if(idw==0){
            adj = ELM_PLIST(digraph, w);
            PLAIN_LIST(adj);
            level++;
            frames += 3; 
            frames[0] = w; // vertex
            frames[1] = 1; // index
            frames[2] = (UInt) adj;
            SET_ELM_PLIST(stack1, ++end1, INTOBJ_INT(w));
            stack2[++end2] = end1;
            SET_ELM_PLIST(id, w, INTOBJ_INT(end1)); 
          } else {
            while (stack2[end2] > idw) {
              end2--; // pop from stack2
            }
          }
        }
      }
    }
  }

  for (v = 1; v <= n; v++) {
    SET_ELM_PLIST(id, v, INTOBJ_INT(INT_INTOBJ(ELM_PLIST(id, v)) - n));
  }

  out = NEW_PREC(2);
  SHRINK_PLIST(comps, LEN_PLIST(comps));
  AssPRec(out, RNamName("id"), id);
  AssPRec(out, RNamName("comps"), comps);
  CHANGED_BAG(out);
  free(stack2);
  return out;
}

static UInt UF_FIND(UInt *id, UInt i) {
  while (i != id[i])
    i = id[i];
  return i;
}

static void UF_COMBINE_CLASSES(UInt *id, UInt i, UInt j) {
  i = UF_FIND(id, i);
  j = UF_FIND(id, j);
  if (i < j)
    id[j] = i;
  else if (j < i)
    id[i] = j;
  // if i = j then there is nothing to combine
}


static Obj FuncDIGRAPH_CONNECTED_COMPONENTS(Obj self, Obj digraph) {
  UInt n, *id, *nid, i, j, e, len, f, nrcomps;
  Obj  adj, adji, source, range, gid, gcomps, comp, out;

  out = NEW_PREC(2);
  n = DigraphNrVertices(digraph);
  if (n == 0) {
    gid = NEW_PLIST(T_PLIST_EMPTY+IMMUTABLE,0);
    gcomps = NEW_PLIST(T_PLIST_EMPTY+IMMUTABLE,0);
  } else {

    id = malloc( n * sizeof(UInt) );
    for (i = 0; i < n; i++) {
      id[i] = i;
    }

    adj = OutNeighbours(digraph);
    for (i = 0; i < n; i++) {
      adji = ELM_PLIST(adj, i + 1);
      PLAIN_LIST(adji);
      len = LEN_PLIST(adji);
      for (e = 1; e <= len; e++) {
        UF_COMBINE_CLASSES(id, i, INT_INTOBJ(ELM_PLIST( adji, e )) - 1);
      }
    }

    // "Normalise" id, giving it sensible labels
    nid = malloc(n * sizeof(UInt));
    nrcomps = 0;
    for (i = 0; i < n; i++) {
      f = UF_FIND(id, i);
      nid[i] = (f == i) ? ++nrcomps : nid[f];
    }
    free(id);

    // Make GAP object from nid
    gid = NEW_PLIST(T_PLIST_CYC+IMMUTABLE, n);
    gcomps = NEW_PLIST(T_PLIST_CYC+IMMUTABLE, nrcomps);
    SET_LEN_PLIST(gid, n);
    SET_LEN_PLIST(gcomps, nrcomps);
    for (i = 1; i <= nrcomps; i++) {
      SET_ELM_PLIST(gcomps, i, NEW_PLIST(T_PLIST_CYC+IMMUTABLE,0));
      CHANGED_BAG(gcomps);
      SET_LEN_PLIST(ELM_PLIST(gcomps, i), 0);
    }
    for (i = 1; i <= n; i++) {
      SET_ELM_PLIST(gid, i, INTOBJ_INT(nid[i - 1]));
      comp = ELM_PLIST(gcomps, nid[i - 1]);
      len = LEN_PLIST(comp);
      AssPlist(comp, len + 1, INTOBJ_INT(i));
    }
    free(nid);

  }
  AssPRec(out, RNamName("id"), gid);
  AssPRec(out, RNamName("comps"), gcomps);
  CHANGED_BAG(out);
  return out;
}

static Obj FuncIS_ACYCLIC_DIGRAPH(Obj self, Obj adj) { 
  UInt  nr, i, j, k, level;
  Obj   nbs;
  UInt  *stack, *ptr;
  
  nr = LEN_PLIST(adj);

  //init the buf
  ptr = calloc( nr + 1, sizeof(UInt) );
  stack =  malloc( (2 * nr + 2) * sizeof(UInt) );
  
  for (i = 1; i <= nr; i++) {
    nbs = ELM_PLIST(adj, i);
    if (LEN_LIST(nbs) == 0) {
      ptr[i] = 1;
    } else if (ptr[i] == 0) {
      level = 1;
      stack[0] = i;
      stack[1] = 1;
      while (1) {
        j = stack[0];
        k = stack[1];
        if (ptr[j] == 2) {
          free(ptr);
          stack -= (2 * level) - 2; // put the stack back to the start
          free(stack);
          return False;  // We have just travelled around a cycle
        }
        // Check whether:
        // 1. We've previously finished with this vertex, OR 
        // 2. Whether we've now investigated all descendant branches
        nbs = ELM_PLIST(adj, j);
        if( ptr[j] == 1 || k > (UInt) LEN_LIST(nbs)) {
          ptr[j] = 1;
          level--;
          if (level == 0) { 
            break;
          }
          // Backtrack and choose next available branch
          stack -= 2;
          ptr[stack[0]] = 0; 
          stack[1]++;
        } else { //Otherwise move onto the next available branch
          ptr[j] = 2; 
          level++;
          nbs = ELM_PLIST(adj, j);
          stack += 2;
          stack[0] = INT_INTOBJ(ADDR_OBJ(nbs)[k]);
          stack[1] = 1;
        }
      }
    }
  }
  free(ptr);
  free(stack);
  return True;
}

static Obj FuncDIGRAPHS_IS_REACHABLE(Obj self, Obj adj, Obj u, Obj v) { 
  UInt  nr, i, j, k, level, target;
  Obj   nbs;
  UInt  *stack, *ptr;

  i      = INT_INTOBJ(u);
  nbs    = ELM_PLIST(adj, i);
  if (LEN_LIST(nbs) == 0) {
    return False;
  }
  target = INT_INTOBJ(v);
  nr     = LEN_PLIST(adj);

  //init the buf
  ptr = calloc( nr + 1, sizeof(UInt) );
  stack =  malloc( (2 * nr + 2) * sizeof(UInt) );

  level = 1;
  stack[0] = i;
  stack[1] = 1;
  while (1) {
    j = stack[0];
    k = stack[1];
    // Check whether:
    // 1. We've previously visited with this vertex, OR 
    // 2. Whether we've now investigated all descendant branches
    nbs = ELM_PLIST(adj, j);
    if( ptr[j] != 0 || k > (UInt) LEN_LIST(nbs)) {
      ptr[j] = 1;
      level--;
      if (level == 0) { 
        break;
      }
      // Backtrack and choose next available branch
      stack -= 2;
      ptr[stack[0]] = 0; 
      stack[1]++;
    } else { //Otherwise move onto the next available branch
      ptr[j] = 2; 
      level++;
      nbs = ELM_PLIST(adj, j);
      stack += 2;
      stack[0] = INT_INTOBJ(ADDR_OBJ(nbs)[k]);
      if (stack[0] == target) {
        free(ptr);
        stack -= (2 * level) - 2; // put the stack back to the start
        free(stack);
        return True;
      }
      stack[1] = 1;
    }
  }
  free(ptr);
  free(stack);
  return False;
}

static Obj FuncIS_ANTISYMMETRIC_DIGRAPH(Obj self, Obj adj) {
  Int  nr, i, j, k, l, level, last1, last2;
  Obj   nbs;
  UInt  *stack, *ptr;
  
  nr = LEN_PLIST(adj);
  if (nr <= 1) {
    return True;
  }

  //init the buf (is this correct length?)
  ptr = calloc( nr + 1, sizeof(UInt) );
  stack =  malloc( (4 * nr + 4) * sizeof(UInt) );
  
  for (i = 1; i <= nr; i++) {
    nbs = ELM_PLIST(adj, i);
    if (LEN_LIST(nbs) == 0) {
      ptr[i] = 1;
    } else if (ptr[i] == 0) {
      level = 1;
      stack[0] = i;
      stack[1] = 1;
      stack[2] = 0;
      stack[3] = 0;
      while (1) {
        j = stack[0];
        k = stack[1];
        last1 = stack[2];
        last2 = stack[3];
        if (j == last2 && j != last1) {
          free(ptr);
          stack -= (4 * level) - 4; // put the stack back to the start
          free(stack);
          return False;  // Found a non-loop cycle of length 2 
        }
        // Check whether:
        // 1. We've previously finished with this vertex, OR 
        // 2. Whether we've now investigated all descendant branches
        nbs = ELM_PLIST(adj, j);
        if( ptr[j] == 2 ) {
          PLAIN_LIST(nbs);
          for ( l = 1; l <= LEN_PLIST(nbs); l++ ) {
            if ( last1 != j && INT_INTOBJ(ADDR_OBJ(nbs)[l]) == last1 ) {
              return False;
            }
          }
        }
        if ( k > LEN_LIST(nbs) ) {
          ptr[j] = 1;
        }
        if( ptr[j] >= 1 ) {
          level--;
          if (level == 0) { 
            break;
          }
          // Backtrack and choose next available branch
          stack -= 4;
          ptr[stack[0]] = 0; 
          stack[1]++;
        } else { //Otherwise move onto the next available branch
          ptr[j] = 2;
          level++;
          nbs = ELM_PLIST(adj, j);
          stack += 4;
          stack[0] = INT_INTOBJ(ADDR_OBJ(nbs)[k]);
          stack[1] = 1;
          stack[2] = j; // I am wasting memory here, duplicating info
          stack[3] = last1;
        }
      }
    }
  }
  free(ptr);
  free(stack);
  return True;
}

static Obj FuncIS_STRONGLY_CONNECTED_DIGRAPH(Obj self, Obj digraph) { 
  UInt n, nextid, *bag, *ptr1, *ptr2, *fptr, *id, w;

  n = LEN_PLIST(digraph);
  if (n == 0){
    return True;
  }

  nextid = 1;
  bag = malloc(n * 4 * sizeof(UInt));
  ptr1 = bag;
  ptr2 = bag + n;
  fptr = bag + n * 2;
  id = calloc(n + 1, sizeof(UInt));

  // first vertex v=1
  PLAIN_LIST(ELM_PLIST(digraph, 1));
  fptr[0] = 1; // vertex
  fptr[1] = 1; // index
  *ptr1 = 1; 
  *ptr2 = nextid;
  id[1] = nextid;
  
  while (1) { // we always return before level = 0
    if (fptr[1] > (UInt) LEN_PLIST(ELM_PLIST(digraph, fptr[0]))) {
      if (*ptr2 == id[fptr[0]]) {
        do {
          n--;
        } while (*(ptr1--) != fptr[0]);
        free(bag);
        free(id);
        return n==0 ? True : False;
      }
      fptr -= 2;
    } else {
      w = INT_INTOBJ(ELM_PLIST(ELM_PLIST(digraph, fptr[0]), fptr[1]++));
      if(id[w] == 0){
        PLAIN_LIST(ELM_PLIST(digraph, w));
        fptr += 2; 
        fptr[0] = w; // vertex
        fptr[1] = 1; // index
        nextid++;
        *(++ptr1) = w; 
        *(++ptr2) = nextid;
        id[w] = nextid;
      } else {
        while ((*ptr2) > id[w]) {
          ptr2--;
        }
      }
    }
  }
  //this should never happen, just to keep the compiler happy
  return Fail;
}

static Obj FuncDIGRAPH_TOPO_SORT(Obj self, Obj adj) {
  UInt  nr, i, j, k, count;
  UInt  level;
  Obj   buf, nbs, out;
  UInt  *stack, *ptr;
  
  nr = LEN_PLIST(adj);
  
  if (nr == 0) {
    return adj;
  }
  out = NEW_PLIST(T_PLIST_CYC+IMMUTABLE, nr);
  SET_LEN_PLIST(out, nr);
  if (nr == 1) {
    SET_ELM_PLIST(out, 1, INTOBJ_INT(1));
    return out;
  }

  //init the buf
  ptr = calloc( nr + 1, sizeof(UInt) );
  stack =  malloc( (2 * nr + 2) * sizeof(UInt) );
  count = 0;

  for (i = 1; i <= nr; i++) {
    nbs = ELM_PLIST(adj, i);
    if (LEN_LIST(nbs) == 0) {
      if (ptr[i] == 0) {
        count++;
        SET_ELM_PLIST(out, count, INTOBJ_INT(i));
      }
      ptr[i] = 1;
    } else if (ptr[i] == 0) {
      level = 1;
      stack[0] = i;
      stack[1] = 1;
      while (1) {
        j = stack[0];
        k = stack[1];
        if (ptr[j] == 2) {
          //SetIsAcyclicDigraph(graph, false);
          stack -= 2;
          level--;
          if ( stack[0] != j ) { //Cycle is not just a loop
            free(ptr);
            stack -= (2 * level) - 2; 
            free(stack);
            return Fail;
          }
          stack[1]++;
          ptr[j] = 0;
          continue;
        }
        nbs = ELM_PLIST(adj, j);
        if( ptr[j] == 1 || k > (UInt) LEN_LIST(nbs)) {
          if ( ptr[j] == 0 ) {
            // ADD J TO THE END OF OUR LIST
            count++;
            SET_ELM_PLIST(out, count, INTOBJ_INT(j));
          }
          ptr[j] = 1;
          level--;
          if (level==0) {
            break;
          }
          // Backtrack and choose next available branch
          stack -= 2;
          ptr[stack[0]] = 0;
          stack[1]++;
        } else { //Otherwise move onto the next available branch
          ptr[j] = 2;
          level++;
          nbs = ELM_PLIST(adj, j);
          stack += 2;
          stack[0] = INT_INTOBJ(ADDR_OBJ(nbs)[k]);
          stack[1] = 1;
        }
      }
    } 
  }
  free(ptr);
  free(stack);
  return out;
}

static Obj FuncDIGRAPH_SOURCE_RANGE(Obj self, Obj digraph) {
  Obj   source, range, adj, adji;
  Int   i, j, k, m, n, len;

  m      = DigraphNrEdges(digraph);
  n      = DigraphNrVertices(digraph);
  adj    = OutNeighbours(digraph); 
  source = NEW_PLIST(T_PLIST_CYC+IMMUTABLE, m);
  range  = NEW_PLIST(T_PLIST_CYC+IMMUTABLE, m);
  SET_LEN_PLIST(source, m);
  SET_LEN_PLIST(range, m);

  k = 0;
  for ( i = 1; i <= n; i++ ) {
    adji = ELM_PLIST( adj, i );
    len    = LEN_LIST( adji );
    for ( j = 1; j <= len; j++ ) {
      k++;
      SET_ELM_PLIST( source, k, INTOBJ_INT( i ) );
      SET_ELM_PLIST( range,  k, ELM_LIST( adji, j ) );
    }
  }
  AssPRec(digraph, RNamName("source"), source);
  AssPRec(digraph, RNamName("range"),  range);
  return True;
}

// Assume we are passed a GAP Int nrvertices
// Two GAP lists of PosInts (all <= nrvertices) of equal length
static Obj FuncDIGRAPH_OUT_NBS(Obj self, Obj nrvertices, Obj source, Obj range) { 
  Obj   adj, adjj;
  UInt  n, m, i, j, len, m1, m2;
 
  m1 = LEN_LIST(source);
  m2 = LEN_LIST(range);
  if (m1 != m2) {
    ErrorQuit("Digraphs: DIGRAPH_OUT_NBS: usage,\n<source> and <range> must be lists of equal length,", 0L, 0L);
  }
  n = INT_INTOBJ(nrvertices);
  if (n == 0) {
    adj = NEW_PLIST(T_PLIST_EMPTY+IMMUTABLE, 0);
  } else {
 
    PLAIN_LIST(source);
    PLAIN_LIST(range);

    adj = NEW_PLIST(T_PLIST_TAB+IMMUTABLE, n);
    SET_LEN_PLIST(adj, n);
    
    // fill adj with empty plists 
    for (i = 1; i <= n; i++) {
      SET_ELM_PLIST(adj, i, NEW_PLIST(T_PLIST_EMPTY+IMMUTABLE, 0));
      SET_LEN_PLIST(ELM_PLIST(adj, i), 0);
      CHANGED_BAG(adj);
    }
    
    n = m1;
    for (i = 1; i <= n; i++) {
      j = INT_INTOBJ(ELM_PLIST(source, i));
      adjj = ELM_PLIST(adj, j);
      len = LEN_PLIST(adjj); 
      if (len == 0){
        RetypeBag(adjj, T_PLIST_CYC+IMMUTABLE);
        CHANGED_BAG(adj);
      }
      AssPlist(adjj, len + 1,  ELM_PLIST(range, i));
    }
  }

  //AssPRec(digraph, RNamName("adj"), adj);
  return adj;
}

// Function to change Out-Neighbours to In-Neighbours, and vice versa
static Obj FuncDIGRAPH_IN_OUT_NBS(Obj self, Obj adj) { 
  Obj   inn, innk, innj, adji;
  UInt  n, m, i, j, k, len, len2;
  
  n = LEN_PLIST(adj);
  if (n == 0) {
    inn = NEW_PLIST(T_PLIST_EMPTY+IMMUTABLE, 0);
  } else {
    inn = NEW_PLIST(T_PLIST_TAB+IMMUTABLE, n);
    SET_LEN_PLIST(inn, n);

    // fill adj with empty plists 
    for (i = 1; i <= n; i++) {
      SET_ELM_PLIST(inn, i, NEW_PLIST(T_PLIST_EMPTY+IMMUTABLE, 0));
      SET_LEN_PLIST(ELM_PLIST(inn, i), 0);
      CHANGED_BAG(inn);
    }

    for (i = 1; i <= n; i++){
      adji = ELM_PLIST(adj, i);
      len = LEN_PLIST(adji);
      for (j = 1; j <= len; j++){
        k = INT_INTOBJ(ELM_PLIST(adji, j));
        innk = ELM_PLIST(inn, k);
        len2 = LEN_PLIST(innk); 
        if(len2 == 0){
          RetypeBag(innk, T_PLIST_CYC+IMMUTABLE);
          CHANGED_BAG(inn);
        }
        AssPlist(innk, len2 + 1, INTOBJ_INT(i));
      }
    }
  }
  return inn;
}

static Obj FuncADJACENCY_MATRIX(Obj self, Obj digraph) {
  Int   n, i, j, val, len, outj;
  Obj   adj, mat, adji, next;

  n = DigraphNrVertices(digraph);
  if (n == 0) {
    return NEW_PLIST(T_PLIST_EMPTY+IMMUTABLE, 0);
  }
  
  adj = OutNeighbours(digraph);
  mat = NEW_PLIST(T_PLIST_TAB+IMMUTABLE, n);
  SET_LEN_PLIST(mat, n);

  for (i = 1; i <= n; i++) {
    // Set up the i^th row of the adjacency matrix
    next = NEW_PLIST(T_PLIST_CYC+IMMUTABLE, n);
    SET_LEN_PLIST(next, n);
    for (j = 1; j <= n; j++) {
      SET_ELM_PLIST(next, j, INTOBJ_INT(0));
    }
    // Fill in the correct values of the matrix
    adji = ELM_PLIST(adj, i);
    len = LEN_LIST(adji);
    for (j = 1; j <= len; j++) {
      outj = INT_INTOBJ( ELM_LIST( adji, j ) );
      val = INT_INTOBJ( ELM_PLIST(next, outj ) ) + 1;
      SET_ELM_PLIST(next, outj, INTOBJ_INT(val));
    }
    SET_ELM_PLIST(mat, i, next);
    CHANGED_BAG(mat);
  }
  SET_FILT_LIST(mat, FN_IS_RECT);
  return mat;
}

static Obj FuncIS_MULTI_DIGRAPH(Obj self, Obj digraph) {
  Obj   adj, adji;
  int   n, i, k, j, jj;
 
  adj = OutNeighbours(digraph); 
  n = DigraphNrVertices(digraph);
  for (i = 1; i <= n; i++) {
    adji = ELM_PLIST(adj, i);
    PLAIN_LIST(adji);
    for (j = 2; j <= LEN_PLIST(adji); j++) {
      jj = INT_INTOBJ(ELM_PLIST(adji, j));
      for (k = 1; k < j; k++) {
        if (INT_INTOBJ(ELM_PLIST(adji, k)) == jj) {
          return True;
        }
      }
    }
  }
  return False;
} 


/***************** GENERAL FLOYD_WARSHALL ALGORITHM***********************
 * This function accepts 5 arguments:
 *   1. A digraph.
 *   2. A special function which takes 5 arguments:
 *       - The matrix dist
 *       - 3 integers i, j, k
 *       - An integer n (the number of vertices of digraph)
 *      and modifies the matrix dist according to the values of i, j, k.
 *   3. Int val1 s.t initially dist[i][j] = val1 if [ i, j ] isn't an edge.
 *   4. Int val2 s.t initially dist[i][j] = val2 if [ i, j ] is an edge.
 *   5. bool copy:
 *      - If false, proceeds as usual Floyd-Warshall algorithm and returns
 *        a GAP object matrix as the result.
 *      - If true, FLOYD_WARSHALL stores the initialised dist, and
 *        compares it with dist after it has gone through the 3 for loops,
 *        and returns true iff it is unchanged.
 */
static Obj FLOYD_WARSHALL(Obj digraph, 
                          void (*func)(Int** dist,
                                       Int   i,
                                       Int   j,
                                       Int   k, 
                                       Int   n),
                          Int  val1, 
                          Int  val2,
                          bool copy,
                          bool diameter) {
  Int   n, i, j, k, *dist, *adj;
  Obj   next, out, outi, val;

  n = DigraphNrVertices(digraph);

  if (n == 0) {
    return NEW_PLIST(T_PLIST_EMPTY+IMMUTABLE, 0);
  }
  dist = malloc( n * n * sizeof(Int) );

  for (i = 0; i < n * n; i++) {
    dist[i] = val1;
  }

  out = OutNeighbours(digraph); 
  for (i = 1; i <= n; i++) {
    outi = ELM_PLIST(out, i);
    PLAIN_LIST(outi);
    for (j = 1; j <= LEN_PLIST(outi); j++) {
      k = (i - 1) * n + INT_INTOBJ(ELM_PLIST(outi, j)) - 1;
      dist[k] = val2;
    } 
  }

  if ( copy ) {
    // This is the special case for IS_TRANSITIVE_DIGRAPH
    adj = malloc( n * n * sizeof(Int) );
    for ( i = 0; i < n * n; i++ ) {
      adj[i] = dist[i];
    }
    // memcpy( (void*)adj, (void*)dist, n * n * sizeof(Int) );
  }
  
  for (k = 0; k < n; k++) {
    for (i = 0; i < n; i++) {
      for (j = 0; j < n; j++) {
        func(&dist, i, j, k, n);
      }
    }
  }

  // the following is a horrible hack
  if ( diameter ) {
    Int maximum = -1;
    for ( i = 0; i < n; i++ ) {
      for ( j = 0; j < n; j++ ) {
        if ( i != j ) {
          if ( dist[i * n + j] > maximum ) {
            maximum = dist[i * n + j];
          } else if ( dist[i * n + j] == -1 ) {
            return INTOBJ_INT(-1);
          }
        }
      }
    }
    free(dist);
    return INTOBJ_INT(maximum);
  }

  if ( copy ) {
    // This is the special case for IS_TRANSITIVE_DIGRAPH
    for ( i = 0; i < n * n; i++) {
      if ( adj[ i ] != dist[ i ] ) {
        free(dist);
        free(adj);
        return False;
      }
    }
    free(dist);
    free(adj);
    return True;
  }
  
  out = NEW_PLIST(T_PLIST_TAB, n);
  SET_LEN_PLIST(out, n);

  for (i = 1; i <= n; i++) {
    next = NEW_PLIST(T_PLIST_CYC, n);
    SET_LEN_PLIST(next, n);
    for (j = 1; j <= n; j++) {
      val = INTOBJ_INT(dist[ (i - 1) * n + (j - 1) ]);
      SET_ELM_PLIST(next, j, val);
    }
    SET_ELM_PLIST(out, i, next);
    CHANGED_BAG(out);
  }
  SET_FILT_LIST(out, FN_IS_RECT);

  free(dist);
  return out;
}

void FW_FUNC_SHORTEST_DIST(Int** dist, Int i, Int j, Int k, Int n) {
  if((*dist)[i * n + k] != -1 && (*dist)[k * n + j] != -1){
    if ((*dist)[i * n + j] == -1 || 
        (*dist)[i * n + j] > (*dist)[i * n + k] + (*dist)[k * n + j]) {
      (*dist)[i * n + j] = (*dist)[i * n + k] + (*dist)[k * n + j];
    }
  }
}

static Obj FuncDIGRAPH_SHORTEST_DIST(Obj self, Obj digraph) {
  return FLOYD_WARSHALL(digraph, FW_FUNC_SHORTEST_DIST, -1, 1, false, false);
}

static Obj FuncDIGRAPH_DIAMETER(Obj self, Obj digraph) {
  return FLOYD_WARSHALL(digraph, FW_FUNC_SHORTEST_DIST, -1, 1, false, true);
}

void FW_FUNC_TRANS_CLOSURE(Int** dist, Int i, Int j, Int k, Int n) {
  if ((*dist)[i * n + k] != 0 && (*dist)[k * n + j] != 0) {
    (*dist)[i * n + j] = 1;
  }
}

static Obj FuncIS_TRANSITIVE_DIGRAPH(Obj self, Obj digraph) {
  return FLOYD_WARSHALL(digraph, FW_FUNC_TRANS_CLOSURE, 0, 1, true, false);
}

static Obj FuncDIGRAPH_TRANS_CLOSURE(Obj self, Obj digraph) {
  return FLOYD_WARSHALL(digraph, FW_FUNC_TRANS_CLOSURE, 0, 1, false, false);
}

void FW_FUNC_REFLEX_TRANS_CLOSURE(Int** dist, Int i, Int j, Int k, Int n) {
  if ((i == j) || ((*dist)[i * n + k] != 0 && (*dist)[k * n + j] != 0)) {
    (*dist)[i * n + j] = 1;
  }
}

static Obj FuncDIGRAPH_REFLEX_TRANS_CLOSURE(Obj self, Obj digraph) {
  return FLOYD_WARSHALL(digraph, FW_FUNC_REFLEX_TRANS_CLOSURE, 0, 1, false, false);
}

static Obj FuncRANDOM_DIGRAPH(Obj self, Obj nn, Obj limm) {
  UInt n, i, j, k, lim;
  Int  len;
  Obj  adj, adji;

  n   = INT_INTOBJ(nn);
  lim = INT_INTOBJ(limm);
  adj = NEW_PLIST(T_PLIST_TAB, n);
  SET_LEN_PLIST(adj, n);
  
  for (i = 1; i <= n; i++) {
    SET_ELM_PLIST(adj, i, NEW_PLIST(T_PLIST_EMPTY, 0));
    SET_LEN_PLIST(ELM_PLIST(adj, i), 0);
    CHANGED_BAG(adj);
  }

  for (i = 1; i <= n; i++) {
    for (j = 1; j <= n; j++) {
      k = rand() % 10000;
      if (k < lim) {
        adji  = ELM_PLIST(adj, i);
        len   = LEN_PLIST(adji);
        if (len == 0) {
          RetypeBag(adji, T_PLIST_CYC);
          CHANGED_BAG(adj);
        }
        AssPlist(adji, len + 1,  INTOBJ_INT(j));
      }
    }
  }
  return adj;
}

static Obj FuncRANDOM_MULTI_DIGRAPH(Obj self, Obj nn, Obj mm) {
  UInt n, m, i, j, k;
  Int  len;
  Obj  adj, adjj;

  n = INT_INTOBJ(nn);
  m = INT_INTOBJ(mm);
  adj = NEW_PLIST(T_PLIST_TAB, n);
  SET_LEN_PLIST(adj, n);
  
  for (i = 1; i <= n; i++) {
    SET_ELM_PLIST(adj, i, NEW_PLIST(T_PLIST_EMPTY, 0));
    SET_LEN_PLIST(ELM_PLIST(adj, i), 0);
    CHANGED_BAG(adj);
  }

  for (i = 1; i <= m; i++) {
    j = (rand() % n) + 1;
    k = (rand() % n) + 1;
    adjj  = ELM_PLIST(adj, j);
    len   = LEN_PLIST(adjj);
    if (len == 0) {
      RetypeBag(adjj, T_PLIST_CYC);
      CHANGED_BAG(adj);
    }
    AssPlist(adjj, len + 1,  INTOBJ_INT(k));
  }
  return adj;
}

bool EqJumbledPlists(Obj l, Obj r, Int nr, Int *buf ) {
  bool eq;
  Int  j, jj;

  // Check first whether the lists are identical
  eq = true;
  for (j = 1; j <= nr; j++) {
    jj = INT_INTOBJ(ELM_PLIST( l, j ));
    if ( jj != INT_INTOBJ(ELM_PLIST( r, j ) ) ) {
      eq = false;
      break;
    }
  }
  
  // Otherwise check that they have equal content
  if (!eq) {

    for (j = 1; j <= nr; j++) {
      jj = INT_INTOBJ(ELM_PLIST(l, j)) - 1 ;
      buf[jj]++;
      jj = INT_INTOBJ(ELM_PLIST(r, j)) - 1;
      buf[jj]--;
    }

    for ( j = 1; j <= nr; j++ ) {
      jj = INT_INTOBJ(ELM_PLIST(l, j)) - 1;
      if (buf[jj] != 0) {
        return false;
      }
    }
 
  }
  return true;
}

static Obj FuncDIGRAPH_EQUALS(Obj self, Obj digraph1, Obj digraph2) {
  UInt  i, n1, n2, m1, m2;
  Obj   out1, out2, a, b;
  Int   nr, *buf;

  // Check NrVertices is equal
  n1 = DigraphNrVertices(digraph1);
  n2 = DigraphNrVertices(digraph2);
  if (n1 != n2) {
    return False;
  }

  // Check NrEdges is equal
  m1 = DigraphNrEdges(digraph1);
  m2 = DigraphNrEdges(digraph2);

  if ( m1 != m2 ) {
    return False;
  }

  out1 = OutNeighbours(digraph1);
  out2 = OutNeighbours(digraph2);

  buf = calloc(n1, sizeof(Int));

  // Compare OutNeighbours of each vertex in turn
  for ( i = 1; i <= n1; i++ ) {
    a = ELM_PLIST( out1, i );
    b = ELM_PLIST( out2, i );
    PLAIN_LIST(a);
    PLAIN_LIST(b);

    nr = LEN_PLIST(a);
    // Check that the OutDegrees match
    if ( nr != LEN_PLIST( b ) ) {
      free(buf);
      return False;
    }

    if (!EqJumbledPlists( a, b, nr, buf ) ) {
      free(buf);
      return False;
    }
  }
  free(buf);
  return True;
}

Int LTJumbledPlists(Obj l, Obj r, Int nr1, Int nr2, Int *buf, Int n ) {
  bool eq;
  Int  j, jj, min;

  // Check first whether the lists are identical
  if ( nr1 == nr2 ) {
    eq = true;
    for (j = 1; j <= nr1; j++) {
      jj = INT_INTOBJ(ELM_PLIST( l, j ));
      if ( jj != INT_INTOBJ(ELM_PLIST( r, j ) ) ) {
        eq = false;
        break;
      }
    }
  } else {
    eq = false;
  }
  
  // Otherwise compare their content
  if (!eq) {

    min = nr1 < nr2 ? nr1 : nr2;

    for (j = 1; j <= min; j++) {
      jj = INT_INTOBJ(ELM_PLIST(l, j)) - 1 ;
      buf[jj]++;
      jj = INT_INTOBJ(ELM_PLIST(r, j)) - 1;
      buf[jj]--;
    }

    for (j = min + 1; j <= nr1; j++) {
      jj = INT_INTOBJ(ELM_PLIST(l, j)) - 1 ;
      buf[jj]++;
    }

    for (j = min + 1; j <= nr2; j++) {
      jj = INT_INTOBJ(ELM_PLIST(r, j)) - 1 ;
      buf[jj]--;
    }

    for ( j = 0; j < n; j++ ) {
      if (buf[j] < 0) {
        //Pr("Found difference: range: %d, number: %d\n", j + 1, buf[j]);
        return 2;
      } else if (buf[j] > 0) {
        //Pr("Found difference: range: %d, number: %d\n", j + 1, buf[j]);
        return 1;
      }
    }
 
  }
  return 0;
  // Return 0: l = r (as multisets)
  // Return 1: l < r
  // Return 2: r < l 
}


static Obj FuncDIGRAPH_LT(Obj self, Obj digraph1, Obj digraph2) {
  UInt  i, n1, n2, m1, m2;
  Obj   out1, out2, a, b;
  Int   nr1, nr2, *buf, comp, max, min;

  // Compare NrVertices
  n1 = DigraphNrVertices(digraph1);
  n2 = DigraphNrVertices(digraph2);
  if (n1 < n2) {
    return True;
  } else if (n2 < n1) {
    return False;
  }

  // Compare NrEdges
  m1 = DigraphNrEdges(digraph1);
  m2 = DigraphNrEdges(digraph2);

  if ( m1 < m2 ) {
    return True;
  } else if ( m2 < m1 ) {
    return False;
  }

  out1 = OutNeighbours(digraph1);
  out2 = OutNeighbours(digraph2); 

  buf = calloc(n1, sizeof(Int));

  // Compare Sorted(out1[i]) and Sorted(out2[i]) for each vertex i
  for ( i = 1; i <= n1; i++ ) {
    a = ELM_PLIST( out1, i );
    b = ELM_PLIST( out2, i );
    PLAIN_LIST(a);
    PLAIN_LIST(b);

    nr1 = LEN_PLIST(a);
    nr2 = LEN_PLIST(b);
    max = nr1 < nr2 ? nr2 : nr1;

    // Check whether both vertices have 0 out-degree
    if ( max != 0 ) {
      if ( nr1 == 0 ) {
        free(buf);
        return False;
      } else if ( nr2 == 0 ) {
        free(buf);
        return True;
      }
      // Both vertices have positive out-degree

      // Compare out1[i] and out2[i]
      comp = LTJumbledPlists( a, b, nr1, nr2, buf, n1 );
      if (comp == 1) {
        free(buf);
        return True;
      } else if (comp == 2) {
        free(buf);
        return False;
      }
      // if comp == 0 then the lists are equal, so continue
    }
  }
  free(buf);
  return False;
}

// bliss 

BlissGraph* buildBlissMultiDigraph(Obj digraph) {
  UInt        n, i, j, k, l, nr;
  Obj         adji, adj;
  BlissGraph  *graph;

  n = DigraphNrVertices(digraph);
  graph = bliss_new(n);

  adj = OutNeighbours(digraph);
  for (i = 1; i <= n; i++) {
    adji = ELM_PLIST(adj, i);
    nr = LEN_PLIST(adji);
    for(j = 1; j <= nr; j++) {
      k = bliss_add_vertex(graph, 1);
      l = bliss_add_vertex(graph, 2);
      bliss_add_edge(graph, i - 1, k);
      bliss_add_edge(graph, k, l);
      bliss_add_edge(graph, l, INT_INTOBJ(ELM_PLIST(adji, j)) - 1);
    }
  }
  return graph;
}

void digraph_hook_function(void               *user_param,
	                   unsigned int       N,
	                   const unsigned int *aut        ) {
  UInt4*  ptr;
  Obj     p, gens;
  UInt    i, n;
  
  n   = INT_INTOBJ(ELM_PLIST(user_param, 1));  //the degree
  p   = NEW_PERM4(n);
  ptr = ADDR_PERM4(p);
  
  for(i = 0; i < n; i++){
    ptr[i] = aut[i];
  }
  
  gens = ELM_PLIST(user_param, 2);
  AssPlist(gens, LEN_PLIST(gens) + 1, p);
  CHANGED_BAG(user_param);
}

static Obj FuncDIGRAPH_AUTOMORPHISMS(Obj self, Obj digraph) {
  Obj                 autos, p, out, n;
  BlissGraph          *graph;
  UInt4               *ptr;
  const unsigned int  *canon;
  Int                 i, nr;
  
  graph = buildBlissMultiDigraph(digraph);
  
  autos = NEW_PLIST(T_PLIST, 2);
  n = INTOBJ_INT(DigraphNrVertices(digraph));

  SET_ELM_PLIST(autos, 1, n);
  SET_ELM_PLIST(autos, 2, NEW_PLIST(T_PLIST, 0)); // perms of the vertices
  CHANGED_BAG(autos);
  SET_LEN_PLIST(autos, 2);
  canon = bliss_find_canonical_labeling(graph, digraph_hook_function, autos, 0);
  
  p   = NEW_PERM4(INT_INTOBJ(n));
  ptr = ADDR_PERM4(p);
 
  for(i = 0; i < INT_INTOBJ(n); i++){
    ptr[i] = canon[i];
  }
  
  bliss_release(graph);

  SET_ELM_PLIST(autos, 1, p);

  if (LEN_PLIST(ELM_PLIST(autos, 2)) == 0) {
    AssPlist(ELM_PLIST(autos, 2), 1, IdentityPerm);
  } else {
    SortDensePlist(ELM_PLIST(autos, 2));
    RemoveDupsDensePlist(ELM_PLIST(autos, 2));
  }
  CHANGED_BAG(autos);

  return autos;
}

void multidigraph_hook_function(void               *user_param,
	                        unsigned           int N,
	                        const unsigned int *aut        ) {
  UInt4   *ptr;
  Obj     p, gens;
  UInt    i, n, m;
  bool    stab;
 
  m   = INT_INTOBJ(ELM_PLIST(user_param, 1));  //the nr of vertices

  stab = true;
  for (i = 0; i < m; i++) {
    if (aut[i] != i) {
      stab = false;
    }
  }
  if (stab) { // permutation of the edges
    n   = INT_INTOBJ(ELM_PLIST(user_param, 2));  //the nr of edges
    p   = NEW_PERM4(n);
    ptr = ADDR_PERM4(p);
    for (i = 0; i < n; i++) {
      ptr[i] = (aut[2 * i + m] - m) / 2;
    }
    gens = ELM_PLIST(user_param, 4);
  } else { // permutation of the vertices
    p   = NEW_PERM4(m);
    ptr = ADDR_PERM4(p);
    for (i = 0; i < m; i++) {
      ptr[i] = aut[i];
    }
    gens = ELM_PLIST(user_param, 3);
  }

  AssPlist(gens, LEN_PLIST(gens) + 1, p);
  CHANGED_BAG(user_param);
}

static Obj FuncMULTIDIGRAPH_AUTOMORPHISMS(Obj self, Obj digraph) {
  Obj                 autos, p, out;
  BlissGraph          *graph;
  UInt4               *ptr;
  const unsigned int  *canon;
  Int                 i, n;
  
  graph = buildBlissMultiDigraph(digraph);
  
  autos = NEW_PLIST(T_PLIST, 4);
  SET_ELM_PLIST(autos, 1, INTOBJ_INT(DigraphNrVertices(digraph)));
  CHANGED_BAG(autos);
  SET_ELM_PLIST(autos, 2, INTOBJ_INT(DigraphNrEdges(digraph)));
  SET_ELM_PLIST(autos, 3, NEW_PLIST(T_PLIST, 0)); // perms of the vertices
  CHANGED_BAG(autos);
  SET_ELM_PLIST(autos, 4, NEW_PLIST(T_PLIST, 0)); // perms of the edges
  CHANGED_BAG(autos);

  canon = bliss_find_canonical_labeling(graph, multidigraph_hook_function, autos, 0);
  
  n   = DigraphNrVertices(digraph);
  p   = NEW_PERM4(n);
  ptr = ADDR_PERM4(p);
 
  for(i = 0; i < n; i++){
    ptr[i] = canon[i];
  }
  
  bliss_release(graph);

  SET_ELM_PLIST(autos, 1, p); 
  
  // remove 2nd entry of autos . . .
  memmove((void *) (ADDR_OBJ(autos) + 2), //destination
          (void *) (ADDR_OBJ(autos) + 3), //source
          (size_t) 2 * sizeof(Obj));
  SET_LEN_PLIST(autos, 3);
  CHANGED_BAG(autos);

  if (LEN_PLIST(ELM_PLIST(autos, 2)) == 0) {
    AssPlist(ELM_PLIST(autos, 2), 1, IdentityPerm);
  } else {
    SortDensePlist(ELM_PLIST(autos, 2));
    RemoveDupsDensePlist(ELM_PLIST(autos, 2));
  }
  if (LEN_PLIST(ELM_PLIST(autos, 3)) == 0) {
    AssPlist(ELM_PLIST(autos, 3), 1, IdentityPerm);
  } else {
    SortDensePlist(ELM_PLIST(autos, 3));
    RemoveDupsDensePlist(ELM_PLIST(autos, 3));
  }
  CHANGED_BAG(autos);

  return autos;
}

static Obj FuncDIGRAPH_CANONICAL_LABELING(Obj self, Obj digraph) {
  Obj   p;
  UInt4 *ptr;
  BlissGraph *graph;
  Int   n, i; 
  const unsigned int *canon;
     
  graph = buildBlissMultiDigraph(digraph);
  
  canon = bliss_find_canonical_labeling(graph, 0, 0, 0); 
  
  n   = DigraphNrVertices(digraph);
  p   = NEW_PERM4(n);
  ptr = ADDR_PERM4(p);
 
  for(i = 0; i < n; i++){
      ptr[i] = canon[i];
  }
  bliss_release(graph);

  return p;
} 

static Obj FuncMULTIDIGRAPH_CANONICAL_LABELING(Obj self, Obj digraph) {
  Obj                 p, q, out;
  UInt4               *ptr;
  BlissGraph          *graph;
  Int                 m, n, i; 
  const unsigned int  *canon;
     
  graph = buildBlissMultiDigraph(digraph);
  
  canon = bliss_find_canonical_labeling(graph, 0, 0, 0); 
  
  m   = DigraphNrVertices(digraph);
  p   = NEW_PERM4(m);  // perm of vertices
  ptr = ADDR_PERM4(p);
 
  for(i = 0; i < m; i++){
    ptr[i] = canon[i];
  }
  
  n = DigraphNrEdges(digraph);
  q   = NEW_PERM4(n);  // perm of edges
  ptr = ADDR_PERM4(q);

  for (i = 0; i < n; i++ ) {
    ptr[i] = canon[2 * i + m] - m;
  }
  
  bliss_release(graph);
  
  out = NEW_PLIST(T_PLIST, 2);
  SET_ELM_PLIST(out, 1, p);
  SET_ELM_PLIST(out, 2, q);
  SET_LEN_PLIST(out, 2);
  CHANGED_BAG(out);

  return out;
}

// convert GAP perms to perms
/*static perm as_perm (Obj const x) {
  UInt  deg, i;
  UInt2 *ptr2;
  UInt4 *ptr4;
  perm  out = new_perm();

  if (TNUM_OBJ(x) == T_PERM2) {
    deg = DEG_PERM2(x); 
    ptr2 = ADDR_PERM2(x);
    for (i = 0; i < deg; i++) {
      out[i] = (UIntS) ptr2[i];
    }
  } else if (TNUM_OBJ(x) == T_PERM4) {
    deg = DEG_PERM4(x); 
    ptr4 = ADDR_PERM4(x);
    for (i = 0; i < deg; i++) {
      out[i] = (UIntS) ptr4[i];
    }
  }

  for (; i < nr2; i++) {
    out[i] = i;
  }
  return out;
}

static Obj as_PERM4 (perm const x) {
  Obj           p;
  UIntS  i;
  UInt4         *ptr;
  
  p   = NEW_PERM4(nr2);
  ptr = ADDR_PERM4(p);
 
  for (i = 0; i < nr2; i++) {
    ptr[i] = (UInt4) x[i];
  }
  return p;
}*/

// GAP-level function

static Obj   GAP_FUNC;                // variable to hold a GAP level hook function

void homo_hook_gap (void*        user_param,
	            const UIntS  nr,
	            const UIntS  *map       ) {
  UInt2   *ptr;
  Obj     t;
  UInt    i;

  // copy map into new trans2 
  t   = NEW_TRANS2(nr);
  ptr = ADDR_TRANS2(t);
  
  for (i = 0; i < nr; i++) {
    ptr[i] = map[i];
  }
  CALL_2ARGS(GAP_FUNC, user_param, t);
}

void homo_hook_collect (void*        user_param,
	                const UIntS  nr,
	                const UIntS  *map       ) {
  UInt2   *ptr;
  Obj     t;
  UInt    i;

  // copy map into new trans2 
  t   = NEW_TRANS2(nr);
  ptr = ADDR_TRANS2(t);
  
  for (i = 0; i < nr; i++) {
    ptr[i] = map[i];
  }
   
  AssPlist(user_param, LEN_PLIST(user_param) + 1, t);
  CHANGED_BAG(user_param);
#if DEBUG
  Pr("found %d homomorphism so far\n", (Int) LEN_PLIST(user_param), 0L);
#endif
}

Obj FuncGRAPH_HOMOS (Obj self, Obj args) { 
  unsigned int  i, j, k, hint_arg, nr1, nr2;
  UInt8         max_results_arg;
  bool          *condition;
  Obj           user_param_arg, out, nbs;  
  int           image[MAXVERTS];

  Obj graph1         = ELM_PLIST(args, 1);  // find homomorphisms from graph1 
  Obj graph2         = ELM_PLIST(args, 2);  // to graph2
  Obj hook_gap       = ELM_PLIST(args, 3);  // apply this function to every homomorphism 
                                            // Fail for none
  Obj user_param_gap = ELM_PLIST(args, 4);  // user_param which can be used in the hook
                                            // Fail for none
  Obj limit_gap      = ELM_PLIST(args, 5);  // the maximum number of results
  Obj hint_gap       = ELM_PLIST(args, 6);  // the maximum rank of a result
  Obj isinjective    = ELM_PLIST(args, 7);  // only consider injective homomorphism
  Obj image_gap      = ELM_PLIST(args, 8);  // only consider homos with image <image>
  Obj kernel         = ELM_PLIST(args, 9);  // only consider homos with kernel <kernel>
  Obj partial_map    = ELM_PLIST(args, 10); // only look for extensions of <partial_map>


  if (limit_gap == Fail || !IS_INTOBJ(limit_gap)) {
    max_results_arg = SMALLINTLIMIT;
  } else {
    max_results_arg = INT_INTOBJ(limit_gap);
  }

  if (user_param_gap == Fail || (hook_gap == Fail && !IS_PLIST(user_param_gap))) {
    user_param_arg = NEW_PLIST(T_PLIST, 0);
    SET_LEN_PLIST(user_param_arg, 0);
  } else {
    user_param_arg = user_param_gap;
  }

  if (IS_INTOBJ(hint_gap)) { 
    hint_arg = INT_INTOBJ(hint_gap);
  } else {
    hint_arg = MAXVERTS + 1;
  }

  bool isinjective_c = (isinjective == True ? true : false);
 


  // install out-neighbours for graph1 
  nr1 = DigraphNrVertices(graph1);
  HomosGraph* homos_graph1 = new_homos_graph(nr1);
  out = OutNeighbours(graph1);
  
  for (i = 0; i < nr1; i++) {
    nbs = ELM_PLIST(out, i + 1);
    for (j = 0; j < LEN_LIST(nbs); j++) {
      k = INT_INTOBJ(ELM_LIST(nbs, j + 1)) - 1;
      add_edges_homos_graph(homos_graph1, i, k);
    }
  }
  
  // install out-neighbours for graph2
  nr2 = DigraphNrVertices(graph2);
  HomosGraph* homos_graph2 = new_homos_graph(nr2);
  out = OutNeighbours(graph2);
  
  for (i = 0; i < nr2; i++) {
    nbs = ELM_PLIST(out, i + 1);
    for (j = 0; j < LEN_LIST(nbs); j++) {
      k = INT_INTOBJ(ELM_LIST(nbs, j + 1)) - 1;
      add_edges_homos_graph(homos_graph2, i, k);
    }
  }

  // init image
  image[0] = 0;                                 //number of entries in image (rank)
  if (image_gap != Fail && IS_PLIST(image_gap)) {
    for (i = 0; i < LEN_LIST(image_gap); i++) {
      image[0]++;
      image[image[0]] = INT_INTOBJ(ELM_LIST(image_gap, i + 1));
    }
  }
  
  if (hook_gap == Fail) {
    GraphHomomorphisms(homos_graph1, homos_graph2, homo_hook_collect, user_param_arg,
        max_results_arg, hint_arg, isinjective_c, image); 
  } else {
    GAP_FUNC = hook_gap;
    GraphHomomorphisms(homos_graph1, homos_graph2, homo_hook_gap, user_param_arg,
        max_results_arg, hint_arg, isinjective_c, image);
  }
  
  if (IS_PLIST(user_param_arg) && LEN_PLIST(user_param_arg) == 0 
      && ! TNUM_OBJ(user_param_arg) == T_PLIST_EMPTY) {
    RetypeBag(user_param_arg, T_PLIST_EMPTY);
  }
  
  free_homos_graph(homos_graph1);
  free_homos_graph(homos_graph2);

  return user_param_arg;
}

/*F * * * * * * * * * * * * * initialize package * * * * * * * * * * * * * * */

/******************************************************************************
*V  GVarFuncs . . . . . . . . . . . . . . . . . . list of functions to export
*/

static StructGVarFunc GVarFuncs [] = {

  { "DIGRAPH_NREDGES", 1, "digraph",
    FuncDIGRAPH_NREDGES, 
    "src/digraphs.c:DIGRAPH_NREDGES" },
  
  { "GABOW_SCC", 1, "adj",
    FuncGABOW_SCC, 
    "src/digraphs.c:GABOW_SCC" },

  { "DIGRAPH_CONNECTED_COMPONENTS", 1, "digraph",
    FuncDIGRAPH_CONNECTED_COMPONENTS,
    "src/digraphs.c:DIGRAPH_CONNECTED_COMPONENTS" },

  { "IS_ACYCLIC_DIGRAPH", 1, "adj",
    FuncIS_ACYCLIC_DIGRAPH, 
    "src/digraphs.c:FuncIS_ACYCLIC_DIGRAPH" },
 
  { "IS_ANTISYMMETRIC_DIGRAPH", 1, "adj",
    FuncIS_ANTISYMMETRIC_DIGRAPH, 
    "src/digraphs.c:FuncIS_ANTISYMMETRIC_DIGRAPH" },
 
  { "IS_STRONGLY_CONNECTED_DIGRAPH", 1, "adj",
    FuncIS_STRONGLY_CONNECTED_DIGRAPH, 
    "src/digraphs.c:FuncIS_STRONGLY_CONNECTED_DIGRAPH" },

  { "DIGRAPH_TOPO_SORT", 1, "adj",
    FuncDIGRAPH_TOPO_SORT, 
    "src/digraphs.c:FuncDIGRAPH_TOPO_SORT" },

  { "DIGRAPH_SOURCE_RANGE", 1, "digraph",
    FuncDIGRAPH_SOURCE_RANGE, 
    "src/digraphs.c:FuncDIGRAPH_SOURCE_RANGE" },

  { "DIGRAPH_OUT_NBS", 3, "nrvertices, source, range",
    FuncDIGRAPH_OUT_NBS, 
    "src/digraphs.c:FuncDIGRAPH_OUT_NBS" },
  
  { "DIGRAPH_IN_OUT_NBS", 1, "adj",
    FuncDIGRAPH_IN_OUT_NBS, 
    "src/digraphs.c:FuncDIGRAPH_IN_OUT_NBS" },

  { "ADJACENCY_MATRIX", 1, "digraph",
    FuncADJACENCY_MATRIX, 
    "src/digraphs.c:FuncADJACENCY_MATRIX" },

  { "IS_MULTI_DIGRAPH", 1, "digraph",
    FuncIS_MULTI_DIGRAPH, 
    "src/digraphs.c:FuncIS_MULTI_DIGRAPH" },

  { "DIGRAPH_SHORTEST_DIST", 1, "digraph",
    FuncDIGRAPH_SHORTEST_DIST, 
    "src/digraphs.c:FuncDIGRAPH_SHORTEST_DIST" },

  { "DIGRAPH_DIAMETER", 1, "digraph",
    FuncDIGRAPH_DIAMETER, 
    "src/digraphs.c:FuncDIGRAPH_DIAMETER" },

  { "IS_TRANSITIVE_DIGRAPH", 1, "digraph",
    FuncIS_TRANSITIVE_DIGRAPH,
    "src/digraphs.c:FuncIS_TRANSITIVE_DIGRAPH" },
  
  { "DIGRAPH_TRANS_CLOSURE", 1, "digraph",
    FuncDIGRAPH_TRANS_CLOSURE,
    "src/digraphs.c:FuncDIGRAPH_TRANS_CLOSURE" },

  { "DIGRAPH_REFLEX_TRANS_CLOSURE", 1, "digraph",
    FuncDIGRAPH_REFLEX_TRANS_CLOSURE,
    "src/digraphs.c:FuncDIGRAPH_REFLEX_TRANS_CLOSURE" },

  { "RANDOM_DIGRAPH", 2, "nn, limm",
    FuncRANDOM_DIGRAPH,
    "src/digraphs.c:FuncRANDOM_DIGRAPH" },
 
  { "RANDOM_MULTI_DIGRAPH", 2, "nn, mm",
    FuncRANDOM_MULTI_DIGRAPH,
    "src/digraphs.c:FuncRANDOM_MULTI_DIGRAPH" },

  { "DIGRAPH_EQUALS", 2, "digraph1, digraph2",
    FuncDIGRAPH_EQUALS,
    "src/digraphs.c:FuncDIGRAPH_EQUALS" },
  
  { "DIGRAPH_LT", 2, "digraph1, digraph2",
    FuncDIGRAPH_LT,
    "src/digraphs.c:FuncDIGRAPH_LT" },

  { "DIGRAPHS_IS_REACHABLE", 3, "digraph, u, v",
    FuncDIGRAPHS_IS_REACHABLE,
    "src/digraphs.c:FuncDIGRAPHS_IS_REACHABLE" },

  { "DIGRAPH_AUTOMORPHISMS", 1, "digraph",
    FuncDIGRAPH_AUTOMORPHISMS, 
    "src/digraphs.c:FuncDIGRAPH_AUTOMORPHISMS" },
 
  { "MULTIDIGRAPH_AUTOMORPHISMS", 1, "digraph",
    FuncMULTIDIGRAPH_AUTOMORPHISMS, 
    "src/digraphs.c:FuncMULTIDIGRAPH_AUTOMORPHISMS" },

  { "DIGRAPH_CANONICAL_LABELING", 1, "digraph",
    FuncDIGRAPH_CANONICAL_LABELING, 
    "src/digraphs.c:FuncDIGRAPH_CANONICAL_LABELING" },
  
  { "MULTIDIGRAPH_CANONICAL_LABELING", 1, "digraph",
    FuncMULTIDIGRAPH_CANONICAL_LABELING, 
    "src/digraphs.c:FuncMULTIDIGRAPH_CANONICAL_LABELING" },

  { "GRAPH_HOMOS", 10, "graph1, graph2, hook, user_param, limit, hint, isinjective, image, kernel, partial_map",
    FuncGRAPH_HOMOS,
    "src/digraphs.c:FuncGRAPH_HOMOS" },

  /*{ "C_STAB_CHAIN", 1, "gens, lmp",
    FuncC_STAB_CHAIN,
    "src/digraphs.c:FuncC_STAB_CHAIN" },*/
  
  /*{ "STAB", 2, "gens, pt",
    FuncSTAB,
    "src/digraphs.c:FuncC_STAB" },*/

  { 0 }

};

/******************************************************************************
*F  InitKernel( <module> )  . . . . . . . . initialise kernel data structures
*/
static Int InitKernel ( StructInitInfo *module )
{
    /* init filters and functions                                          */
    InitHdlrFuncsFromTable( GVarFuncs );

    /* return success                                                      */
    return 0;
}

/******************************************************************************
*F  InitLibrary( <module> ) . . . . . . .  initialise library data structures
*/
static Int InitLibrary ( StructInitInfo *module )
{
    Int             i, gvar;
    Obj             tmp;

    /* init filters and functions */
    for ( i = 0;  GVarFuncs[i].name != 0;  i++ ) {
      gvar = GVarName(GVarFuncs[i].name);
      AssGVar(gvar,NewFunctionC( GVarFuncs[i].name, GVarFuncs[i].nargs,
                                 GVarFuncs[i].args, GVarFuncs[i].handler ) );
      MakeReadOnlyGVar(gvar);
    }

    tmp = NEW_PREC(0);
    gvar = GVarName("DIGRAPHS_C"); 
    AssGVar( gvar, tmp ); 
    MakeReadOnlyGVar(gvar);

    /* return success                                                      */
    return 0;
}

/******************************************************************************
*F  InitInfopl()  . . . . . . . . . . . . . . . . . table of init functions
*/
static StructInitInfo module = {
#ifdef DIGRAPHSSTATIC
 /* type        = */ MODULE_STATIC,
#else
 /* type        = */ MODULE_DYNAMIC,
#endif
 /* name        = */ "digraphs",
 /* revision_c  = */ 0,
 /* revision_h  = */ 0,
 /* version     = */ 0,
 /* crc         = */ 0,
 /* initKernel  = */ InitKernel,
 /* initLibrary = */ InitLibrary,
 /* checkInit   = */ 0,
 /* preSave     = */ 0,
 /* postSave    = */ 0,
 /* postRestore = */ 0
};

#ifndef DIGRAPHSSTATIC
StructInitInfo * Init__Dynamic ( void )
{
  return &module;
}
#endif

StructInitInfo * Init__digraphs ( void )
{
  return &module;
}

/*
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

