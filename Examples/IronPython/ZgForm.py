import clr
clr.AddReference('System.Drawing')
clr.AddReference('System.Windows.Forms')
clr.AddReference('ZedGraph')

from System.Windows.Forms import (Application, AutoScaleMode, Form, Keys, MouseButtons)
from System.Drawing import (Point, Size, SizeF, Color)
from ZedGraph import ZedGraphControl

# A ZedGraphControl that has zooming etc. enabled.
class ZgControl(ZedGraphControl):
    def __init__(self):
        self.zgc = ZedGraphControl()
        self.zgc.EditButtons = MouseButtons.Left
        self.zgc.EditModifierKeys = (Keys.Alt | Keys.None)
        self.zgc.IsAutoScrollRange = False
        self.zgc.IsEnableHEdit = False
        self.zgc.IsEnableHPan = True
        self.zgc.IsEnableHZoom = True
        self.zgc.IsEnableVEdit = False
        self.zgc.IsEnableVPan = True
        self.zgc.IsEnableVZoom = True
        self.zgc.IsPrintFillPage = True
        self.zgc.IsPrintKeepAspectRatio = True
        self.zgc.IsScrollY2 = False
        self.zgc.IsShowContextMenu = True
        self.zgc.IsShowCopyMessage = True
        self.zgc.IsShowCursorValues = False
        self.zgc.IsShowHScrollBar = False
        self.zgc.IsShowPointValues = False
        self.zgc.IsShowVScrollBar = False
        self.zgc.IsSynchronizeXAxes = False
        self.zgc.IsSynchronizeYAxes = False
        self.zgc.IsZoomOnMouseCenter = False
        self.zgc.LinkButtons = MouseButtons.Left
        self.zgc.LinkModifierKeys = (Keys.Alt | Keys.None)
        self.zgc.Location = Point(12, 12)
        self.zgc.Name = "zgc"
        self.zgc.PanButtons = MouseButtons.Left
        self.zgc.PanButtons2 = MouseButtons.Middle
        self.zgc.PanModifierKeys = (Keys.Shift | Keys.None)
        self.zgc.PanModifierKeys2 = Keys.None
        self.zgc.PointDateFormat = 'g'
        self.zgc.PointValueFormat = 'G'
        self.zgc.ScrollMaxX = 0
        self.zgc.ScrollMaxY = 0
        self.zgc.ScrollMaxY2 = 0
        self.zgc.ScrollMinX = 0
        self.zgc.ScrollMinY = 0
        self.zgc.ScrollMinY2 = 0
        self.zgc.Size = Size(499, 333)
        self.zgc.TabIndex = 0
        self.zgc.ZoomButtons = MouseButtons.Left
        self.zgc.ZoomButtons2 = MouseButtons.None
        self.zgc.ZoomModifierKeys = Keys.None
        self.zgc.ZoomModifierKeys2 = Keys.None
        self.zgc.ZoomStepFraction = 0.1


# A standard form with a ZgControl in it.
class ZgForm(Form):
    def __init__(self):
        self.zgControl = ZgControl()
        
        self.AutoScaleDimensions = SizeF(6.0, 13.0)
        self.AutoScaleMode = AutoScaleMode.Font
        self.ClientSize = Size(523, 357)
        self.Controls.Add(self.zgControl)
        self.Name = 'ScatterPlot'
        self.Text = 'Scatter Plot'
        self.Resize += self.Form1_Resize
        self.Load += self.Form1_Load
        
    #Form1_Load() creates and loads the graph
    def Form1_Load(self, object, event):
    
        # Calculate the Axis Scale Ranges
        self.SetSize()
        self.zgControl.AxisChange()
        self.zgControl.Invalidate()
        
    def Form1_Resize(self, object, event):
        self.SetSize()

    def SetSize(self):
        self.zgControl.Location = Point(10, 10)
        # Leave a small margin around the outside of the control
        self.zgControl.Size = Size((self.ClientRectangle.Width - 20), 
                             (self.ClientRectangle.Height - 20))
                                            
    # Respond to a Zoom Event
    def MyZoomEvent(self, control, oldState, newState):
        pass
        # Here we get notification everytime the user zooms
        
