# A script for modifying a PFS file, in this case a MIKE Zero toolbox file

import clr
import sys

# The SetupLatest method will make your script find the MIKE assemblies at runtime.
# This is required for MIKE Version 2019 (17.0) and onwards. For previous versions, the 
# next three lines must be removed.
clr.AddReference("DHI.Mike.Install");
from DHI.Mike.Install import MikeImport, MikeProducts
MikeImport.SetupLatest(MikeProducts.MikeCore)

clr.AddReference("DHI.PFS");
from DHI.PFS import *

# Loading/Reading the file
pfsFile = PFSFile(sys.argv[1], False);

# Outmost section
target = pfsFile.GetTarget("MzTxStat", 1);

# Sub-section
section = target.GetSection("Setup", 1);

# Modify file name parameter
section.GetKeyword("InputFileName", 1).GetParameter(1).ModifyFileNameParameter(sys.argv[2]);
# Modify integer parameter
section.GetKeyword("GridLimits", 1).GetParameter(3).ModifyIntParameter(int(sys.argv[3]));
section.GetKeyword("GridLimits", 1).GetParameter(4).ModifyIntParameter(int(sys.argv[4]));

# Write back file
pfsFile.Write(sys.argv[1]);
