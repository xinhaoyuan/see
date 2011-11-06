/* Static generic implementation for the core routines of Red Black
 * Tree by Xinhao.Yuan(xinhaoyuan at gmail dot com). Copyright of the
 * code is dedicated to the Public Domain */

/* This code is deviated from the rbt_algo.hpp, a template version of
 * RBTree */

/* Still in BETA stage */

/*
 * ChangeLog:

 * 2011-05-24:

 * - Add __RBT_NeedFixUp switch for performance issue, now could be
     faster than STL in some situations

 * 2011-05-23:

 * - Internal ``insert'' and ``remove'' routines added.

 * 2011-05-20:

 * - Initial version

 */

#ifndef __RBT_NodeType
#error  __RBT_NodeType not defined
#endif

#ifndef __RBT_KeyType
#error  __RBT_KeyType not defined
#endif

#ifndef __RBT_NodeNull
#error  __RBT_NodeNull not defined
#endif

#ifndef __RBT_TouchNode
#error  __RBT_TouchNode(n) not defined
#endif

#ifndef __RBT_NewRedNode
#error  __RBT_NewRedNode(k) not defined
#endif

#ifndef __RBT_SwapNodeContent
#error  __RBT_SwapNodeContent(a,b) not defined
#endif

#ifndef __RBT_GetRank
#error  __RBT_GetRank(n) not defined
#endif

#ifndef __RBT_SetRank
#error  __RBT_SetRank(n,r) not defined
#endif

#ifndef __RBT_CompareKey
#error  __RBT_CompareKey(k,n) not defined
#endif

#ifndef __RBT_AcquireParentAndDir
#error  __RBT_AcquireParentAndDir(n,p,d) not defined
#endif

#ifndef __RBT_GetLeftChild
#error  __RBT_GetLeftChild(n) not defined
#endif

#ifndef __RBT_GetRightChild
#error  __RBT_GetRightChild(n) not defined
#endif

#ifndef __RBT_SetLeftChild
#error  __RBT_SetLeftChild(n,c) not defined
#endif

#ifndef __RBT_SetRightChild
#error  __RBT_SetRightChild(n,c) not defined
#endif

#ifndef __RBT_SetLeftChildFromRightChild
#error  __RBT_SetLeftChildFromRightChild(n,p) not defined
#endif

#ifndef __RBT_SetRightChildFromLeftChild
#error  __RBT_SetRightChildFromLeftChild(n,p) not defined
#endif

#ifndef __RBT_ThrowException
#error  __RBT_ThrowException(msg) not defined
#endif

#define DIR_UNDEFINED 0
#define DIR_ROOT      1
#define DIR_LEFT      2
#define DIR_RIGHT     3

int
__RBT_RedFixUp(__RBT_NodeType *root, __RBT_NodeType node)
{
	 __RBT_NodeType parent;
	 __RBT_NodeType left;
	 __RBT_NodeType right;
	 
	 int fix_dir = __RBT_GetRank(node) > 0 ? DIR_UNDEFINED : DIR_ROOT;
	 int dir;
	 __RBT_AcquireParentAndDir(node, parent, dir);
	 
	 while (1)
	 {
		  switch (dir)
		  {
		  case DIR_ROOT:
			   if (__RBT_GetRank(node) < 0)
					__RBT_SetRank(node, -__RBT_GetRank(node) + 1);
			   *root = node;
			   return 0;
			   break;
			   
		  case DIR_LEFT:
			   __RBT_SetLeftChild(parent, node);

			   left = node;
			   right = __RBT_GetRightChild(parent);
			   
			   node = parent;
			   __RBT_AcquireParentAndDir(node, parent, dir);
			   
			   switch (fix_dir)
			   {
			   case DIR_UNDEFINED:
#ifdef __RBT_NeedFixup
					fix_dir = DIR_UNDEFINED;
#else
					return 0;
#endif
					break;

			   case DIR_ROOT:
					if (__RBT_GetRank(node) > 0)
						 fix_dir = DIR_UNDEFINED;
					else fix_dir = DIR_LEFT;
					break;

			   case DIR_LEFT:
					if (__RBT_GetRank(right) < 0)
					{
						 __RBT_SetRank(left, __RBT_GetRank(node));
						 __RBT_SetRank(right, __RBT_GetRank(node));
						 __RBT_SetRank(node, -__RBT_GetRank(node));

						 fix_dir = DIR_ROOT;
					}
					else
					{
						 __RBT_SetLeftChildFromRightChild(node, left);
						 __RBT_SetRightChild(left, node);

						 __RBT_SetRank(left, __RBT_GetRank(node));
						 __RBT_SetRank(node, -__RBT_GetRank(node) + 1);

						 node = left;
						 fix_dir = DIR_UNDEFINED;
					}
					break;

			   case DIR_RIGHT:
					if (__RBT_GetRank(right) < 0)
					{
						 __RBT_SetRank(left, __RBT_GetRank(node));
						 __RBT_SetRank(right, __RBT_GetRank(node));
						 __RBT_SetRank(node, -__RBT_GetRank(node));
						 
						 fix_dir = DIR_ROOT;
					}
					else
					{
						 __RBT_NodeType left_right = __RBT_GetRightChild(left);

						 // Double rotation

						 __RBT_SetLeftChildFromRightChild(node, left_right);
						 __RBT_SetRightChildFromLeftChild(left, left_right);

						 __RBT_SetLeftChild(left_right, left);
						 __RBT_SetRightChild(left_right, node);

						 __RBT_SetRank(left_right, __RBT_GetRank(node));
						 __RBT_SetRank(node, -__RBT_GetRank(node) + 1);

						 node = left_right;

						 fix_dir = DIR_UNDEFINED;
					}
					break;
			   }
			   break;
			   
		  case DIR_RIGHT:
			   __RBT_SetRightChild(parent, node);

			   left = __RBT_GetLeftChild(parent);
			   right = node;
			   
			   node = parent;
			   __RBT_AcquireParentAndDir(node, parent, dir);
			   
			   switch (fix_dir)
			   {
			   case DIR_UNDEFINED:
#ifdef __RBT_NeedFixup
					fix_dir = DIR_UNDEFINED;
#else
					return 0;
#endif
					break;
					
			   case DIR_ROOT:
					if (__RBT_GetRank(node) > 0)
						 fix_dir = DIR_UNDEFINED;
					else fix_dir = DIR_RIGHT;
					break;

			   case DIR_RIGHT:
					if (__RBT_GetRank(left) < 0)
					{
						 __RBT_SetRank(right, __RBT_GetRank(node));
						 __RBT_SetRank(left, __RBT_GetRank(node));
						 __RBT_SetRank(node, -__RBT_GetRank(node));

						 fix_dir = DIR_ROOT;
					}
					else
					{
						 __RBT_SetRightChildFromLeftChild(node, right);
						 __RBT_SetLeftChild(right, node);

						 __RBT_SetRank(right, __RBT_GetRank(node));
						 __RBT_SetRank(node, -__RBT_GetRank(node) + 1);

						 node = right;
						 fix_dir = DIR_UNDEFINED;
					}
					break;

			   case DIR_LEFT:
					if (__RBT_GetRank(left) < 0)
					{
						 __RBT_SetRank(right, __RBT_GetRank(node));
						 __RBT_SetRank(left, __RBT_GetRank(node));
						 __RBT_SetRank(node, -__RBT_GetRank(node));
						 
						 fix_dir = DIR_ROOT;
					}
					else
					{
						 __RBT_NodeType right_left = __RBT_GetLeftChild(right);

						 // Double rotation

						 __RBT_SetRightChildFromLeftChild(node, right_left);
						 __RBT_SetLeftChildFromRightChild(right, right_left);

						 __RBT_SetRightChild(right_left, right);
						 __RBT_SetLeftChild(right_left, node);

						 __RBT_SetRank(right_left, __RBT_GetRank(node));
						 __RBT_SetRank(node, -__RBT_GetRank(node) + 1);

						 node = right_left;

						 fix_dir = DIR_UNDEFINED;
					}
					break;
			   }
			   break;

		  }
	 }
}

int
__RBT_BlackFixUp(__RBT_NodeType *root, __RBT_NodeType parent, int dir)
{
	 __RBT_NodeType node;
	 __RBT_NodeType tmp_node;
	 __RBT_NodeType left;
	 __RBT_NodeType right;
	 __RBT_NodeType c_left;
	 __RBT_NodeType c_right;

	 int fix_dir = DIR_ROOT;
	 if (dir == DIR_LEFT)
		  node = __RBT_GetLeftChild(parent);
	 else if (dir == DIR_RIGHT)
		  node = __RBT_GetRightChild(parent);
	 else __RBT_ThrowException("???");

	 while (1)
	 {
		  switch (dir)
		  {
		  case DIR_ROOT:
			   if (__RBT_GetRank(node) < 0)
					__RBT_SetRank(node, -__RBT_GetRank(node) + 1);
			   *root = node;
			   return 0;
			   break;
			   
		  case DIR_LEFT:
			   __RBT_SetLeftChild(parent, node);

			   left = node;
			   right = __RBT_GetRightChild(parent);
			   
			   node = parent;
			   __RBT_AcquireParentAndDir(node, parent, dir);

			   switch (fix_dir)
			   {
			   case DIR_UNDEFINED:
#ifdef __RBT_NeedFixup
					fix_dir = DIR_UNDEFINED;
#else
					return 0;
#endif
					break;
					
			   case DIR_ROOT:
					if (__RBT_GetRank(left) < 0)
					{
						 __RBT_SetRank(left,  -__RBT_GetRank(left) + 1);
						 fix_dir = DIR_UNDEFINED;
					}
					else
					{
						 int promote = 0;
						 if (__RBT_GetRank(right) < 0)
						 {
							  // Single rotation, non-stop
							  
							  c_left = __RBT_GetLeftChild(right);

							  __RBT_SetRank(right, __RBT_GetRank(node));
							  __RBT_SetRank(node, -__RBT_GetRank(node) + 1);

							  __RBT_SetRightChild(node, c_left);

							  tmp_node = right;
							  right = c_left;
							  
							  promote = 1;
						 }

						 if (__RBT_GetRank(right) < 0)
							  __RBT_ThrowException("???");
						 
						 // Process when sibling is black
						 c_left = __RBT_GetLeftChild(right);
						 c_right = __RBT_GetRightChild(right);

						 if (__RBT_GetRank(c_left) > 0 && __RBT_GetRank(c_right) > 0)
						 {
							  if (__RBT_GetRank(node) < 0)
							  {
								   __RBT_SetRank(node, -__RBT_GetRank(node));
								   __RBT_SetRank(right, -__RBT_GetRank(right) + 1);

								   if (promote)
								   {
										__RBT_SetLeftChild(tmp_node, node);
										node = tmp_node;
								   }
								   fix_dir = DIR_UNDEFINED;
							  }
							  else
							  {
								   __RBT_SetRank(node, __RBT_GetRank(node) - 1);
								   __RBT_SetRank(right, -__RBT_GetRank(right) + 1);

								   fix_dir = DIR_ROOT;
							  }
						 }
						 else if (__RBT_GetRank(c_right) < 0)
						 {
							  __RBT_SetRank(c_right, __RBT_GetRank(right));
							  __RBT_SetRank(right, __RBT_GetRank(node));
							  __RBT_SetRank(node, __RBT_GetRank(c_right));

							  // Single rotation
							  __RBT_SetRightChildFromLeftChild(node, right);
							  __RBT_SetLeftChild(right, node);

							  if (promote)
							  {
								   __RBT_SetLeftChild(tmp_node, right);
								   node = tmp_node;
							  }
							  else node = right;
							  fix_dir = DIR_UNDEFINED;
						 }
						 else
						 {
							  __RBT_SetRank(c_left, __RBT_GetRank(node));
							  __RBT_SetRank(node, __RBT_GetRank(right));
								   
							  // Double rotation
							  __RBT_SetRightChildFromLeftChild(node, c_left);
							  __RBT_SetLeftChildFromRightChild(right, c_left);
							  __RBT_SetLeftChild(c_left, node);
							  __RBT_SetRightChild(c_left, right);

							  if (promote)
							  {
								   __RBT_SetLeftChild(tmp_node, c_left);
								   node = tmp_node;
							  }
							  else node = c_left;
							  fix_dir = DIR_UNDEFINED;
						 }
					}
					break;
			   default:
					__RBT_ThrowException("???");
					break;
			   }
			   
			   break;
			   
		  case DIR_RIGHT:
			   __RBT_SetRightChild(parent, node);

			   left = __RBT_GetLeftChild(parent);
			   right = node;
			   
			   node = parent;
			   __RBT_AcquireParentAndDir(node, parent, dir);

			   switch (fix_dir)
			   {
			   case DIR_UNDEFINED:
#ifdef __RBT_NeedFixup
					fix_dir = DIR_UNDEFINED;
#else
					return 0;
#endif
					break;
					
			   case DIR_ROOT:
					if (__RBT_GetRank(right) < 0)
					{
						 __RBT_SetRank(right,  -__RBT_GetRank(right) + 1);
						 fix_dir = DIR_UNDEFINED;
					}
					else
					{
						 int promote = 0;
						 if (__RBT_GetRank(left) < 0)
						 {
							  // Single rotation, non-stop
							  
							  c_right = __RBT_GetRightChild(left);

							  __RBT_SetRank(left, __RBT_GetRank(node));
							  __RBT_SetRank(node, -__RBT_GetRank(node) + 1);

							  __RBT_SetLeftChild(node, c_right);

							  tmp_node = left;
							  left = c_right;
							  
							  promote = 1;
						 }

						 if (__RBT_GetRank(left) < 0)
							  __RBT_ThrowException("???");
						 
						 // Process when sibling is black
						 c_right = __RBT_GetRightChild(left);
						 c_left = __RBT_GetLeftChild(left);

						 if (__RBT_GetRank(c_right) > 0 && __RBT_GetRank(c_left) > 0)
						 {
							  if (__RBT_GetRank(node) < 0)
							  {
								   __RBT_SetRank(node, -__RBT_GetRank(node));
								   __RBT_SetRank(left, -__RBT_GetRank(left) + 1);

								   if (promote)
								   {
										__RBT_SetRightChild(tmp_node, node);
										node = tmp_node;
								   }
								   fix_dir = DIR_UNDEFINED;
							  }
							  else
							  {
								   __RBT_SetRank(node, __RBT_GetRank(node) - 1);
								   __RBT_SetRank(left, -__RBT_GetRank(left) + 1);

								   fix_dir = DIR_ROOT;
							  }
						 }
						 else if (__RBT_GetRank(c_left) < 0)
						 {
							  __RBT_SetRank(c_left, __RBT_GetRank(left));
							  __RBT_SetRank(left, __RBT_GetRank(node));
							  __RBT_SetRank(node, __RBT_GetRank(c_left));

							  // Single rotation
							  __RBT_SetLeftChildFromRightChild(node, left);
							  __RBT_SetRightChild(left, node);

							  if (promote)
							  {
								   __RBT_SetRightChild(tmp_node, left);
								   node = tmp_node;
							  }
							  else node = left;
							  fix_dir = DIR_UNDEFINED;
						 }
						 else
						 {
							  __RBT_SetRank(c_right, __RBT_GetRank(node));
							  __RBT_SetRank(node, __RBT_GetRank(left));
								   
							  // Double rotation
							  __RBT_SetLeftChildFromRightChild(node, c_right);
							  __RBT_SetRightChildFromLeftChild(left, c_right);
							  __RBT_SetRightChild(c_right, node);
							  __RBT_SetLeftChild(c_right, left);

							  if (promote)
							  {
								   __RBT_SetRightChild(tmp_node, c_right);
								   node = tmp_node;
							  }
							  else node = c_right;
							  fix_dir = DIR_UNDEFINED;
						 }
					}
					break;
			   default:
					__RBT_ThrowException("???");
					break;
			   }
			   break;
		  }
	 }
}

__RBT_NodeType
__RBT_Insert(__RBT_NodeType root, __RBT_NodeType *node, __RBT_KeyType key)
{
	 if (root == __RBT_NodeNull)
	 {
		  *node =__RBT_NewRedNode(key);
		  __RBT_RedFixUp(&root, *node);
	 }
	 else
	 {
		  __RBT_NodeType cur = root;
		  while (1)
		  {
			   int c = __RBT_CompareKey(key, cur);
			   if (c == 0)
			   {
					*node = cur;
					break;
			   }
			   else if (c == -1)
			   {
					if (__RBT_GetLeftChild(cur) == __RBT_NodeNull)
					{
						 *node = __RBT_NewRedNode(key);
						 __RBT_SetLeftChild(cur, *node);
						 __RBT_RedFixUp(&root, *node);
						 break;
					}
					else cur = __RBT_GetLeftChild(cur);
			   }
			   else
			   {
					if (__RBT_GetRightChild(cur) == __RBT_NodeNull)
					{
						 *node = __RBT_NewRedNode(key);
						 __RBT_SetRightChild(cur, *node);
						 __RBT_RedFixUp(&root, *node);
						 break;
					}
					else cur = __RBT_GetRightChild(cur);
			   }
		  }
	 }

	 return root;
}

__RBT_NodeType
__RBT_Remove(__RBT_NodeType root, __RBT_NodeType *node, __RBT_KeyType key)
{
	 if (root == __RBT_NodeNull)
		  return root;

	 __RBT_NodeType cur = root;
	 __RBT_NodeType parent = __RBT_NodeNull;
	 int dir;
	 while (1)
	 {
		  int cmp = __RBT_CompareKey(key, cur);
		  if (cmp == 0)
		  {
			   if (__RBT_GetLeftChild(cur) == __RBT_NodeNull)
			   {
					/* No Left Child -- We can delete the node directly */
					*node = cur;
					cur = __RBT_GetRightChild(*node);

					if (__RBT_GetRank(*node) < 0)
					{
						 /* KILL RED 1 */
						 if (dir == DIR_LEFT)
							  __RBT_SetLeftChild(parent, cur);
						 else __RBT_SetRightChild(parent, cur);
						 /* FIXME */
						 return root;
					}
					else if (__RBT_GetRank(cur) < 0)
					{
						 /* Kill RED 2 */
						 __RBT_SetRank(cur, __RBT_GetRank(*node));
						 if (parent == __RBT_NodeNull)
							  return cur;
						 else if (dir == DIR_LEFT)
							  __RBT_SetLeftChild(parent, cur);
						 else __RBT_SetRightChild(parent, cur);
						 /* FIXME */
						 return root;
					}
					else
					{
						 if (parent == __RBT_NodeNull)
							  return cur;
						 else if (dir == DIR_LEFT)
							  __RBT_SetLeftChild(parent, cur);
						 else __RBT_SetRightChild(parent, cur);

						 __RBT_BlackFixUp(&root, parent, dir);
						 return root;
					}
			   }
			   else
			   {
					__RBT_NodeType r = cur;
					
					parent = cur;
					cur = __RBT_GetLeftChild(cur);

					if (__RBT_GetRightChild(cur) == __RBT_NodeNull)
					{
						 __RBT_SwapNodeContent(r, cur);

						 *node = cur;
						 cur = __RBT_GetLeftChild(*node);

						 if (__RBT_GetRank(*node) < 0)
						 {
							  /* KILL RED 1 */
							  if (parent == __RBT_NodeNull)
								   return cur;
							  else __RBT_SetLeftChild(parent, cur);
							  /* FIXME */
							  return root;
						 }
						 else if (__RBT_GetRank(cur) < 0)
						 {
							  /* Kill RED 2 */
							  __RBT_SetRank(cur, __RBT_GetRank(*node));
							  if (parent == __RBT_NodeNull)
								   return cur;
							  else __RBT_SetLeftChild(parent, cur);
							  /* FIXME */
							  return root;
						 }
						 else
						 {
							  __RBT_SetLeftChild(parent, cur);
							  __RBT_BlackFixUp(&root, parent, DIR_LEFT);
							  return root;
						 }
					}
					
					while (__RBT_GetRightChild(cur) != __RBT_NodeNull)
					{
						 parent = cur;
						 cur = __RBT_GetRightChild(cur);
					}

					__RBT_SwapNodeContent(r, cur);

					*node = cur;
					cur = __RBT_GetLeftChild(*node);

					if (__RBT_GetRank(*node) < 0)
					{
						 /* KILL RED 1 */
						 __RBT_SetRightChild(parent, cur);
						 /* FIXME */
						 return root;
					}
					else if (__RBT_GetRank(cur) < 0)
					{
						 /* Kill RED 2 */
						 __RBT_SetRank(cur, __RBT_GetRank(*node));
						 __RBT_SetRightChild(parent, cur);
						 /* FIXME */
						 return root;
					}
					else
					{
						 __RBT_SetRightChild(parent, cur);
						 __RBT_BlackFixUp(&root, parent, DIR_RIGHT);
						 return root;
					}
			   }
		  }
		  else if (cmp == -1)
		  {
			   parent = cur;
			   dir = DIR_LEFT;
			   cur = __RBT_GetLeftChild(cur);
			   if (cur == __RBT_NodeNull) return root;
		  }
		  else
		  {
			   parent = cur;
			   dir = DIR_RIGHT;
			   cur = __RBT_GetRightChild(cur);
			   if (cur == __RBT_NodeNull) return root;
		  }
	 }
}
