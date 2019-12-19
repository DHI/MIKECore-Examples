using System;
using System.Collections.Generic;
using System.Linq;
using DHI.Generic.MikeZero;
using DHI.Generic.MikeZero.DFS.mesh;

namespace DHI.MikeCore.Examples
{

  /// <summary>
  /// Class to merge several meshes into one big mesh.
  /// <para>
  /// The meshes must share one or more boundaries, and
  /// the nodes on the shared boundary must match within <see cref="NodeTolerance"/>
  /// in the two meshes.
  /// </para>
  /// <para>
  /// By default only nodes with a boundary code will be tried for merging. If setting also
  /// <see cref="TryMergeAllNodes"/>, also nodes without a boundary code will be tried for
  /// merging.
  /// </para>
  /// </summary>
  /// <remarks>
  /// This MeshMerger class has a <see cref="Main"/> method, and hence it can be used
  /// as an entry point for a stand-alone application. See the <see cref="Main"/> method
  /// for examples on how to use the class.
  /// <para>
  /// Check out the MeshMergerBuild.bat on how to build this from the command line.
  /// </para>
  /// </remarks>
  public class MeshMerger
  {
    /// <summary> Static constructor </summary>
    static MeshMerger()
    {
      // The setup method will make your application find the MIKE assemblies at runtime.
      // The first call of the setup method takes precedense. Any subsequent calls will be ignored.
      // It must be called BEFORE any method using MIKE libraries is called, i.e. it is not sufficient
      // to call it as the first thing in that method using the MIKE libraries. Often this can be achieved
      // by having this code in the static constructor.
      // If MIKE Core is x-copy deployed with the application, this is not required.
      if (!DHI.Mike.Install.MikeImport.Setup(17, DHI.Mike.Install.MikeProducts.MikeCore))
        throw new Exception("Cannot find a proper MIKE installation");
    }

    /// <summary> 
    /// When false, only try merge on boundary nodes, i.e. internal nodes will never be merged. 
    /// When true, mergin will also be tried on internal nodes
    /// </summary>
    public bool TryMergeAllNodes { get; set; }
    /// <summary> 
    /// Tolerance distance for when two nodes equals. 
    /// This should never be larger than the smallest face length
    /// (smallest distance between two nodes)
    /// </summary>
    public double NodeTolerance { get; set; }

    /// <summary> 
    /// Number of nodes merged during the process
    /// </summary>
    public int NodeMergeCount { get; set; }

    private readonly string _newMeshFileName;
    private QuadSearchTree _nodeSearchTree;

    // Node variables for merged mesh
    private readonly List<double> _x = new List<double>();
    private readonly List<double> _y = new List<double>();
    private readonly List<double> _z = new List<double>();
    private readonly List<int> _code = new List<int>();

    // Element variables for merged mesh
    private readonly List<int> _elementType = new List<int>();
    private readonly List<int[]> _connectivity = new List<int[]>();

    /// <summary>
    /// Constructor
    /// </summary>
    /// <param name="tol">Sets the <see cref="NodeTolerance"/></param>
    /// <param name="newMeshFileName">New file name of merged mesh</param>
    /// <param name="tryMergeAllNodes">Sets the <see cref="TryMergeAllNodes"/></param>
    public MeshMerger(double tol, string newMeshFileName, bool tryMergeAllNodes)
    {
      _newMeshFileName = newMeshFileName;
      TryMergeAllNodes = tryMergeAllNodes;
      NodeTolerance = tol;
    }

    /// <summary>
    /// Process the incoming mesh file names
    /// </summary>
    /// <param name="files">List of mesh file names to merge</param>
    /// <param name="fileBoundaryCodesToRemove">List of boundary codes to remove for each mesh. Must match the size of the files argument</param>
    public void Process(List<string> files, List<List<int>> fileBoundaryCodesToRemove = null)
    {
      // Extent of entire domain, all meshes
      Extent extent = new Extent();
      
      // Load all meshes
      List<MeshFile> meshes = new List<MeshFile>(files.Count);
      for (int i = 0; i < files.Count; i++)
      {
        MeshFile mesh = MeshFile.ReadMesh(files[i]);
        meshes.Add(mesh);
        for (int j = 0; j < mesh.NumberOfNodes; j++)
        {
          extent.Include(mesh.X[j], mesh.Y[j]);
        }
      }
      // grow it a littl bit, in case of rounding errors
      extent.XMin = extent.XMin - NodeTolerance;
      extent.XMax = extent.XMax + NodeTolerance;
      extent.YMin = extent.YMin - NodeTolerance;
      extent.YMax = extent.YMax + NodeTolerance;

      // Initialize search tree
      _nodeSearchTree = new QuadSearchTree(extent);

      // Create new mesh nodes and elements
      for (int i = 0; i < files.Count; i++)
      {
        int prevNodeMergeCount = NodeMergeCount;
        List<int> boundaryCodesToRemove = fileBoundaryCodesToRemove != null ? fileBoundaryCodesToRemove[i] : null;
        AddMesh(meshes[i], boundaryCodesToRemove);
        if (i > 0)
        {
          Console.Out.WriteLine("Mesh {0}, number of nodes merged in: {1}", i+1, NodeMergeCount-prevNodeMergeCount);
        }
      }
      Console.Out.WriteLine("Total number of nodes merged in  : {0}", NodeMergeCount);

      RemoveInternalBoundaryCodes(_code, _connectivity);

      // Create new mesh file
      string projection = meshes[0].ProjectionString;
      eumQuantity eumQuantity = meshes[0].EumQuantity;

      MeshBuilder builder = new MeshBuilder();
      builder.SetNodes(_x.ToArray(), _y.ToArray(), _z.ToArray(), _code.ToArray());
      builder.SetElements(_connectivity.ToArray());
      builder.SetProjection(projection);
      builder.SetEumQuantity(eumQuantity);

      MeshFile newMesh = builder.CreateMesh();

      MeshValidator meshValidator = new MeshValidator();
      meshValidator.ValidateMesh(newMesh.X, newMesh.Y, newMesh.Code, newMesh.ElementTable);

      foreach (string error in meshValidator.Errors)
      {
        Console.Out.WriteLine(error);
      }
      
      newMesh.Write(_newMeshFileName);

      //-------------------------------------
      // Do some statistics on the mesh:

      // collect number of face codes for each mesh
      SortedDictionary<int, int[]> bcCodesStats = new SortedDictionary<int, int[]>();
      List<MeshValidator> validators = new List<MeshValidator>();
      for (int meshIndex = 0; meshIndex < meshes.Count; meshIndex++)
      {
        MeshFile meshFile = meshes[meshIndex];
        MeshValidator validator = new MeshValidator();
        validator.ValidateMesh(meshFile.X, meshFile.Y, meshFile.Code, meshFile.ElementTable);
        validators.Add(validator);
        UpdateStatistics(meshes.Count + 1, meshIndex, bcCodesStats, validator.GetFaceCodeStatistics());
      }
      UpdateStatistics(meshes.Count + 1, meshes.Count, bcCodesStats, meshValidator.GetFaceCodeStatistics());

      Console.Out.Write("---------------------");
      Console.Out.Write(" Statistics of faces ");
      Console.Out.Write("---------------------");
      Console.Out.WriteLine("");
      Console.Out.Write("FaceCode  |");
      for (int i = 0; i < meshes.Count; i++)
      {
        Console.Out.Write(" mesh {0,2}  ", i+1);
      }
      Console.Out.Write(" |  total | new mesh");
      Console.Out.WriteLine("");

      int[] totals = new int[meshes.Count+2];
      foreach (KeyValuePair<int, int[]> keyValuePair in bcCodesStats)
      {
        Console.Out.Write("    {0,4}  |",keyValuePair.Key);
        int total = 0;
        for (int index = 0; index < keyValuePair.Value.Length-1; index++)
        {
          int meshCodeCount = keyValuePair.Value[index];
          total += meshCodeCount;
          totals[index] += meshCodeCount; 
          Console.Out.Write(" {0,7}  ", meshCodeCount);
        }
        totals[meshes.Count] += total;
        totals[meshes.Count + 1] += keyValuePair.Value.Last();
        Console.Out.Write(" |{0,7} | ", total);
        Console.Out.Write(" {0,7}  ", keyValuePair.Value.Last());
        Console.Out.WriteLine("");
      }
      Console.Out.Write("   total  |");
      for (int index = 0; index < meshes.Count; index++)
      {
        Console.Out.Write(" {0,7}  ", totals[index]);
      }
      Console.Out.Write(" |{0,7} | ", totals[meshes.Count]);
      Console.Out.Write(" {0,7}  ", totals[meshes.Count+1]);
      Console.Out.WriteLine("");
      Console.Out.Write("---------------------");
      Console.Out.Write("---------------------");
      Console.Out.Write("---------------------");
      Console.Out.WriteLine("");
  
    }

    private void UpdateStatistics(int meshCount, int meshIndex, SortedDictionary<int, int[]> stats, List<KeyValuePair<int, int>> data)
    {
      foreach (KeyValuePair<int, int> faceCodeCount in data)
      {
        int[] counts;
        if (!stats.TryGetValue(faceCodeCount.Key, out counts))
        {
          counts = new int[meshCount];
          stats.Add(faceCodeCount.Key, counts);
        }
        counts[meshIndex] += faceCodeCount.Value;
      }
    }

    private void AddMesh(MeshFile mesh, List<int> boundaryCodesToRemove = null)
    {
      
      // node numbers in new mesh for each node in provided mesh
      int[] renumberNodes = new int[mesh.NumberOfNodes];

      // Reused variables
      Extent e = new Extent();
      List<QuadSearchTree.Point> points = new List<QuadSearchTree.Point>();

      // Index of first new node
      int newMeshFirstNodeIndex = _x.Count;

      // Add nodes of current mesh to merged mesh, find which nodes should be merged
      for (int i = 0; i < mesh.NumberOfNodes; i++)
      {
        double x = mesh.X[i];
        double y = mesh.Y[i];
        double z = mesh.Z[i];
        int code = mesh.Code[i];

        bool boundaryNode = code != 0;

        if (boundaryCodesToRemove != null && boundaryCodesToRemove.Contains(code))
          code = 0;

        // This criteria selects which nodes to try to merge on, i.e. 
        // when to try to find a "duplicate" in merged mesh.
        if (TryMergeAllNodes || boundaryNode)
        {
          // Check if this current node already "exists" in the merged mesh
          // i.e. if the current node is "close to" a node in the merged mesh.
          // "Close to" is decided by the NodeTolerance
          
          // Find all nodes within NodeTolerance of (x,y) in search tree
          UpdatePointExtent(e, x, y, NodeTolerance);
          points.Clear();
          _nodeSearchTree.Find(e, points);
          if (points.Count > 0)
          {
            // find the point closest to (x,y), and witin NodeTolerance distance
            QuadSearchTree.Point closest = null;
            // Distance must be less than NodeTolerance in order to reuse existing node
            // Comparing on squared distances, to avoid taking a lot of square roots
            double minDistSq = NodeTolerance * NodeTolerance;
            foreach (QuadSearchTree.Point point in points)
            {
              double dx = point.X - x;
              double dy = point.Y - y;
              double distSq = dx*dx + dy*dy;
              if (distSq < minDistSq)
              {
                closest = point;
                minDistSq = distSq;
              }
            }

            if (closest != null)
            {
              // Node already exist at/close to coordinate, use existing node 
              // and disregard the current node, remember node number (1-based)
              // for renumbering when adding elements from current mesh.
              renumberNodes[i] = closest.No;
              // Update boundary code
              if (_code[closest.No - 1] == 0 && code != 0)
                _code[closest.No - 1] = code;
              // Store some statistics
              NodeMergeCount++;
              // Skip from here, since the current node should not be added
              // to the merged mesh.
              continue;
            }
          }
        }

        // Add node to merged mesh
        _x.Add(x);
        _y.Add(y);
        _z.Add(z);
        _code.Add(code);

        // Number of node (1-based) in new mesh
        int nodeNum = _x.Count;
        // remember new node number, for renumbering when adding elements from current mesh
        renumberNodes[i] = nodeNum;
      }

      // Add all new nodes to the quad search tree. New nodes are first added to the search tree 
      // now, such that the merge routine never merges two nodes from the same mesh.
      for (int i = newMeshFirstNodeIndex; i < _x.Count; i++)
      {
        // Add to search tree
        QuadSearchTree.Point p = new QuadSearchTree.Point()
                                   {
                                     No = i + 1,
                                     X = _x[i],
                                     Y = _y[i],
                                   };
        _nodeSearchTree.Add(p);
      }

      // Add all elements
      for (int i = 0; i < mesh.NumberOfElements; i++)
      {
        _elementType.Add(mesh.ElementType[i]);
        // Renumber the nodes of each element to the new node numbers
        int[] meshElmt = mesh.ElementTable[i];
        int[] newelmt = new int[meshElmt.Length];
        for (int j = 0; j < meshElmt.Length; j++)
        {
          newelmt[j] = renumberNodes[meshElmt[j]-1];
        }
        _connectivity.Add(newelmt);
      }

    }

    /// <summary>
    /// Remove boundary code from internal nodes,
    /// i.e. nodes that are not on the boundary.
    /// </summary>
    private void RemoveInternalBoundaryCodes(List<int> code, List<int[]> connectivity)
    {
      // Store all faces, based on start-node, store end-node
      // Be aware that the connectivity is 1-based, indexing is zero based.
      List<int>[] nodeFaces = new List<int>[code.Count];
      for (int i = 0; i < code.Count; i++)
      {
        nodeFaces[i] = new List<int>();
      }
      for (int i = 0; i < connectivity.Count; i++)
      {
        int[] elmt = connectivity[i];
        for (int j = 0; j < elmt.Length; j++)
        {
          int startNode = elmt[j];
          int endNode   = elmt[(j + 1) % elmt.Length];
          nodeFaces[startNode-1].Add(endNode);
        }
      }

      // Check which faces have a reverse face.
      bool[] boundaryNode = new bool[code.Count];
      for (int i = 0; i < nodeFaces.Length; i++)
      {
        int startNode = i + 1;
        // Loop over all faces
        List<int> faceEndNodes = nodeFaces[i];
        for (int j = 0; j < faceEndNodes.Count; j++)
        {
          int endNode = faceEndNodes[j];
          // Check if "reverse" face is found
          if (!nodeFaces[endNode - 1].Contains(startNode))
          {
            // "Reverse" face was not found, this is a boundary face
            boundaryNode[startNode - 1] = true;
            boundaryNode[endNode - 1] = true;
          }
        }
      }

      // Remove code for all but boundary nodes
      for (int i = 0; i < code.Count; i++)
      {
        if (!boundaryNode[i])
          code[i] = 0;
      }
    }



    private void UpdatePointExtent(Extent e, double x, double y, double tol)
    {
      e.XMin = x - tol;
      e.XMax = x + tol;
      e.YMin = y - tol;
      e.YMax = y + tol;
    }

    /// <summary>
    /// Usage string for command line arguments supported by the <see cref="Main"/> method
    /// </summary>
    static void Usage()
    {
      string usage =
@"
Usage: 
  MeshMerger <options> newmeshfilename meshfilespecs meshfilespecs ....

Options:
  -tol=X           default is 1e-2
  -all             Try merge all nodes. Default is to only merge nodes
                   with a boundary code.

Multiple meshfilespecs can be specified. Format of meshfilespecs:
   mymeshfile.mesh
      Adds mesh to merged mesh file
   mymeshfile.mesh*1,3
      Adds mesh to merged mesh file, removing boundary codes 1 and 3 from mesh

Removal of boundary codes are required when merging a common boundary. Otherwise
boundary nodes can end up in the middle of the mesh.
";
      Console.Out.WriteLine(usage);
    }

    /// <summary>
    /// Main method, used when compiling as a standalone application.
    /// </summary>
    public static int Main(string[] args)
    {
      try
      {
        if (args.Length < 3)
        {
          Usage();
          return -1;
        }

        string newMeshFileName = null;
        double tol = 1e-2;
        bool tryMergeAllNodes = false;
        List<string> files = new List<string>();
        List<List<int>> fileBoundaryCodesToRemove = new List<List<int>>();

        // Parse command line arguments
        for (int i = 0; i < args.Length; i++)
        {
          string arg = args[i];

          if (arg.ToLower().StartsWith("-tol="))
          {
            tol = double.Parse(arg.Substring(5));
          }
          else if (arg.ToLower().StartsWith("-all"))
          {
            tryMergeAllNodes = true;
          }
          else if (newMeshFileName == null)
          {
            // First filename is new output mesh file name
            newMeshFileName = arg;
          }
          else
          {
            // Input mesh specification
            string filename = arg;
            List<int> codesToRemove = new List<int>();

            // check if there are boundary codes to remove specified
            int splitPos = arg.IndexOf('*');
            if (splitPos > 0)
            {
              filename = arg.Substring(0, splitPos);
              string bctrStr = arg.Substring(splitPos + 1);
              // parse boundary codes to remove
              string[] codesStrs = bctrStr.Split(',');
              for (int j = 0; j < codesStrs.Length; j++)
              {
                codesToRemove.Add(int.Parse(codesStrs[j]));
              }
            }
            files.Add(filename);
            fileBoundaryCodesToRemove.Add(codesToRemove);
          }
        }

        if (files.Count < 2)
        {
          Console.Out.WriteLine("At least two input meshes must be specified");
          Usage();
          return -1;
        }

        if (System.IO.File.Exists(newMeshFileName))
        {
          Console.Out.WriteLine("File with new mesh file name already exists. Please specify a new name. Aborting");
          return -1;
        }

        MeshMerger merger = new MeshMerger(tol, newMeshFileName, tryMergeAllNodes);
        merger.Process(files, fileBoundaryCodesToRemove);

        Console.Out.WriteLine("Merging completed, output file: "+newMeshFileName);

      }
      catch (Exception)
      {
        Usage();
        throw;
      }

      return 0;
    }
  }



  /// <summary>
  /// Simple 2D search tree based on a 2D quad tree.
  /// <para>
  /// The search tree structure is build up by adding a number of coordinates
  /// to the search tree
  /// </para>
  /// </summary>
  internal class QuadSearchTree
  {
    public class Point
    {
      public int No;
      public double X;
      public double Y;
    }
    
    /// <summary>
    /// Head of the tree
    /// </summary>
    private readonly TreeNode _head;

    /// <summary>
    /// Number of coordinates in search tree
    /// </summary>
    private int _numPoints;

    /// <summary>
    /// Create a new search tree that covers the provided <paramref name="extent"/>
    /// </summary>
    /// <param name="extent">Extent that the search tree should cover</param>
    public QuadSearchTree(Extent extent)
    {
      _head = new TreeNode(extent);
    }

    /// <summary>
    /// Number of coordinates in search tree
    /// </summary>
    public int Count
    {
      get { return _numPoints; }
    }

    /// <summary>
    /// Add point to the search tree, thereby building the tree.
    /// </summary>
    /// <param name="point">xy point to add</param>
    /// <returns>Returns true on success, false if point already exists in tree</returns>
    public bool Add(Point point)
    {
      bool added = _head.Add(point);
      if (added)
        _numPoints++;
      return added;
    }

    /// <summary>
    /// Find points that is included in the provided <paramref name="extent"/>
    /// </summary>
    /// <param name="extent">Extent to look for points within</param>
    /// <param name="points">List to put points in</param>
    public void Find(Extent extent, List<Point> points)
    {
      _head.Find(extent, points);
    }


    private class TreeNode
    {
      public int MaxPointsPerNode = 10;

      private readonly Extent _extent;
      private TreeNode[] _children;
      private List<Point> _points = new List<Point>();

      public TreeNode(Extent extent)
      {
        _extent = extent;
      }

      private bool HasChildren
      {
        get { return (_children != null); }
      }

      public bool Add(Point point)
      {
        bool added = false;
        // Check if inside this domain
        if (!_extent.Contains(point.X, point.Y))
          return false;

        // If has children, add recursively
        if (HasChildren)
        {
          foreach (TreeNode child in _children)
          {
            added |= child.Add(point);
          }
        }
        else // it does not have children, add it here
        {
          // Check if it already exists
          foreach (Point existingPoint in _points)
          {
            if (point.X == existingPoint.X && point.Y == existingPoint.Y)
              return false; // It did exist, do nothing
          }
          // Add point
          _points.Add(point);
          added = true;

          // Check if we should subdivide
          if (_points.Count > MaxPointsPerNode)
          {
            SubDivide();
          }
        }
        return (added);
      }

      private void SubDivide()
      {
        // Create children
        _children = new TreeNode[4];

        double xMid = 0.5 * (_extent.XMin + _extent.XMax);
        double yMid = 0.5 * (_extent.YMin + _extent.YMax);
        _children[0] = new TreeNode(new Extent(xMid, _extent.XMax, yMid, _extent.YMax));
        _children[1] = new TreeNode(new Extent(_extent.XMin, xMid, yMid, _extent.YMax));
        _children[2] = new TreeNode(new Extent(_extent.XMin, xMid, _extent.YMin, yMid));
        _children[3] = new TreeNode(new Extent(xMid, _extent.XMax, _extent.YMin, yMid));

        // Add points of this node to the new children
        foreach (Point point in _points)
        {
          foreach (TreeNode child in _children)
          {
            child.Add(point);
          }
        }

        // Delete points of this node
        _points.Clear();
        _points = null;

      }


      public void Find(Extent extent, List<Point> elmts)
      {
        // If no overlap, just return
        if (!_extent.Overlaps(extent))
          return;

        // If has children, ask those
        if (HasChildren)
        {
          foreach (TreeNode child in _children)
          {
            child.Find(extent, elmts);
          }
        }
        else // No children, search in elements of this node.
        {
          foreach (Point coor in _points)
          {
            if (extent.Contains(coor.X, coor.Y))
            {
              elmts.Add(coor);
            }
          }
        }
      }
    }
  }

  /// <summary>
  /// Extent is a rectangle in xy-space.
  /// </summary>
  public class Extent
  {
    /// <summary>
    /// Minimum x coordinate of extent
    /// </summary>
    public double XMin;
    /// <summary>
    /// Maximum x coordinate of extent
    /// </summary>
    public double XMax;
    /// <summary>
    /// Minimum y coordinate of extent
    /// </summary>
    public double YMin;
    /// <summary>
    /// Maximum y coordinate of extent
    /// </summary>
    public double YMax;

    /// <summary>
    /// Default constructor, creates an empty extent
    /// </summary>
    public Extent()
    {
      XMin = Double.MaxValue;
      XMax = Double.MinValue;
      YMin = Double.MaxValue;
      YMax = Double.MinValue;
    }

    /// <summary>
    /// Constructor that ininitalizes the extent to a certain extent.
    /// </summary>
    /// <param name="xmin">Minimum x coordinate</param>
    /// <param name="xmax">Maximum x coordinate</param>
    /// <param name="ymin">Minimum y coordinate</param>
    /// <param name="ymax">Maximum y coordinate</param>
    public Extent(double xmin, double xmax, double ymin, double ymax)
    {
      XMin = xmin;
      XMax = xmax;
      YMin = ymin;
      YMax = ymax;
    }

    /// <summary>
    /// Copy constructor that ininitalizes the extent to a certain extent.
    /// </summary>
    public Extent(Extent other)
    {
      XMin = other.XMin;
      XMax = other.XMax;
      YMin = other.YMin;
      YMax = other.YMax;
    }

    /// <summary>
    /// Make this extent include <paramref name="other"/>. This will
    /// grow this extent, if the <paramref name="other"/> point is outside
    /// this extent.
    /// </summary>
    /// <param name="other">Other extent to include</param>
    public void Include(Extent other)
    {
      if (other.XMin < XMin)
        XMin = other.XMin;
      if (other.XMax > XMax)
        XMax = other.XMax;
      if (other.YMin < YMin)
        YMin = other.YMin;
      if (other.YMax > YMax)
        YMax = other.YMax;
    }

    /// <summary>
    /// Make this extent include the xy-point. This will
    /// grow this extent, if the xy-point is outside
    /// this extent.
    /// </summary>
    /// <param name="x">x coordinate of point to include</param>
    /// <param name="y">y coordinate of point to include</param>
    public void Include(double x, double y)
    {
      if (x < XMin)
        XMin = x;
      if (x > XMax)
        XMax = x;
      if (y < YMin)
        YMin = y;
      if (y > YMax)
        YMax = y;
    }

    /// <summary>
    /// Checks if this extent contains the xy-point
    /// </summary>
    /// <param name="x">x coordinate of point to include</param>
    /// <param name="y">y coordinate of point to include</param>
    /// <returns>True if xy-point is inside (or on boundary of) this extent.</returns>
    public bool Contains(double x, double y)
    {
      return (
                 XMin <= x && x <= XMax &&
                 YMin <= y && y <= YMax
             );
    }

    /// <summary>
    /// Checks if this extent overlaps the other extent
    /// </summary>
    /// <param name="other">Extent to check overlap with</param>
    /// <returns>True if the two extends overlaps</returns>
    public bool Overlaps(Extent other)
    {
      return
          (
              XMin <= other.XMax && XMax >= other.XMin &&
              YMin <= other.YMax && YMax >= other.YMin
          );
    }
  }

  /// <summary>
  /// Class for validating mesh. Currently validates faces.
  /// </summary>
  public class MeshValidator
  {
    /// <summary>
    /// For each node, list of all faces starting
    /// from the node. The list for each node contains to-node
    /// indices (0-based) for the face.
    /// <para>
    /// If a face is an internal face, it exists in both
    /// directions, i.a. A->B and B->A.
    /// </para>
    /// </summary>
    public List<int>[] FaceNodes { get; private set; }

    /// <summary>
    /// Validation errors
    /// </summary>
    public List<string> Errors { get; private set; }

    private int _internalFaceCount;
    private int[] _faceCodeCount;

    public void ValidateMesh(double[] xs, double[] ys, int[] codes, int[][] connectivity)
    {
      Errors = new List<string>();
      _internalFaceCount = 0;
      _faceCodeCount = new int[codes.Max()+1];

      BuildFaceInfo(xs.Length, connectivity);
      ValidateFaceInfo(codes);

    }

    private void BuildFaceInfo(int numNodes, int[][] connectivity)
    {
      FaceNodes = new List<int>[numNodes];
      for (int i = 0; i < numNodes; i++)
        FaceNodes[i] = new List<int>();

      for (int i = 0; i < connectivity.Length; i++)
      {
        int[] elmt = connectivity[i];
        for (int j = 0; j < elmt.Length; j++)
        {
          int fromNode = elmt[j]-1;
          int toNode = elmt[(j + 1)%elmt.Length]-1;
          AddFace(fromNode, toNode);
        }
      }
    }

    private void AddFace(int fromNode, int toNode)
    {
      List<int> toNodes = FaceNodes[fromNode];
      if (toNodes.Contains(toNode))
      {
        Errors.Add(string.Format("Invalid mesh: Double face, from node {0} to node {1}. " +
                                 "Hint: Probably too many nodes was merged into one of the two face nodes." +
                                 "Try decrease node merge tolerance value",
                                  fromNode + 1, toNode + 1));
      }
      else
      {
        toNodes.Add(toNode);
      }
    }

    private void ValidateFaceInfo(int[] codes)
    {
      for (int fromNode = 0; fromNode < FaceNodes.Length; fromNode++)
      {
        List<int> toNodes = FaceNodes[fromNode];

        for (int i = 0; i < toNodes.Count; i++)
        {
          int toNode = toNodes[i];

          // If the opposite face exists, this face is an internal face
          if (FaceNodes[toNode].Contains(fromNode))
          {
            FaceInternal();
            continue;
          }

          // Opposite face does not exist, so it is a boundary face.
          int fromCode = codes[fromNode];
          int toCode = codes[toNode];

          // True if "invalid" boundary face, then set it as internal face.
          bool internalFace = false;

          if (fromCode == 0)
          {
            Errors.Add(string.Format("Invalid mesh: Boundary face, from node {0} to node {1} is missing a boundary code on node {0}. " +
                                     "Hint: Modify boundary code for node {0}",
                                     fromNode + 1, toNode + 1));
            internalFace = true;
          }
          if (toCode == 0)
          {
            Errors.Add(string.Format("Invalid mesh: Boundary face, from node {0} to node {1} is missing a boundary code on node {1}. " +
                                     "Hint: Modify boundary code for node {1}",
                                     fromNode + 1, toNode + 1));
            internalFace = true;
          }

          int faceCode;

          // Find face code:
          // 1) In case any of the nodes is a land node (code value 1) then the
          //    boundary face is a land face, given boundary code value 1.
          // 2) For boundary faces (if both fromNode and toNode have code values larger than 1), 
          //    the face code is the boundary code value of toNode.
          if (fromCode == 1 || toCode == 1)
            faceCode = 1;
          else
            faceCode = toCode;

          if (internalFace)
            FaceInternal();
          else
            FaceBoundary(faceCode);
        }
      }
    }

    private void FaceInternal()
    {
      _internalFaceCount++;
    }

    private void FaceBoundary(int code)
    {
      _faceCodeCount[code]++;
    }

    public List<KeyValuePair<int, int>> GetFaceCodeStatistics()
    {
      List<KeyValuePair<int, int>> res = new List<KeyValuePair<int, int>>();
      
      // Divide internal faces with two, since they are counted twice
      res.Add(new KeyValuePair<int, int>(0, _internalFaceCount));
      for (int i = 0; i < _faceCodeCount.Length; i++)
      {
        int c = _faceCodeCount[i];
        if (c > 0)
        {
          res.Add(new KeyValuePair<int, int>(i, c));
        }
      }
      return res;
    }
  }


}