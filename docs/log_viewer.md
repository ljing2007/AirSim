# Log Viewer

The LogViewer is a Windows WPF app that presents the MavLink streams that it is getting from the
Unreal Simulator.  You can use this to monitor what is happening on the drone while it is flying.
For example, the picture below shows  a real time graph of the x, y an z gyro sensor information being generated by the 
simulator.

### Usage

To use this LogViewer, connect the simulator `before` you run the simulation.  Simply press the blue connector
button on the top right corner of the window, select the Socket `tab`, enter the port number 14388, and
your `localhost` network.  Then press the record button (triange on the right hand side of the toolbar).
Now start the simulator, pick some mavlink items to graph,  you  should see something like this:

![Log Viewer](images/log_viewer.png)

The drone view here is the actual estimated position coming from the PX4, so that is a great way to check
whether the PX4 is in sync with the simulator.  Sometimes you can see some drift here as the attitude
estimation catches up with reality, this is more visible after a bad crash.

### Installation

If you can't build the LogViewer.sln, there is also a 
[click once installer](http://www.lovettsoftware.com/LovettSoftware/Downloads/Px4LogViewer/Px4LogViewer.application).

### Configuration

The magic port number 14388 can be configured in the simulator by editing the [settings.json file](settings.md).
