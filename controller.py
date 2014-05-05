import sys
import os

from PyQt4 import QtGui, QtCore

class MyPushButton(QtGui.QPushButton):
    def __init__(self, popMenu,elementID, mainForm):
        super(MyPushButton, self).__init__()
        self.__elementID = elementID
        self.__mainForm = mainForm
        self.__popMenu = popMenu

        self.connect(self, QtCore.SIGNAL('customContextMenuRequested(const QPoint&)'), self.on_context_menu)   
        self.connect(self, QtCore.SIGNAL('clicked()'),  self,        QtCore.SLOT("triggerOutput()"))    

    def on_context_menu(self, point):
        # show context menu
        self.__popMenu.exec_(self.mapToGlobal(point)) 

    @QtCore.pyqtSlot()
    def triggerOutput(self):
        self.__mainForm.emit(QtCore.SIGNAL("buttonXclickedSignal(PyQt_PyObject)"), self.__elementID) # send signal to MainForm class

def stop():
        os.system('pkill ocl-example-fac')
class MainForm(QtGui.QWidget):
    def __init__(self, parent=None):
        super(MainForm, self).__init__(parent)
        self.setGeometry(300, 300, 400, 200)
        VmasterLayout = QtGui.QVBoxLayout(self)
        self.Hbox = QtGui.QHBoxLayout()
	self.setWindowTitle('Control Panel')
        # Custom signal
        self.connect(self, QtCore.SIGNAL("buttonXclickedSignal(PyQt_PyObject)"),         self.buttonXclicked)
	i = sys.argv[1]
        for i in range(0,int(i)):
            # create context menu as you like
            popMenu = QtGui.QMenu(self)
            popMenu.addAction(QtGui.QAction('Load Device %s '%(i), self))
            popMenu.addAction(QtGui.QAction('Load Device %s'%(i), self))
            popMenu.addSeparator()
            popMenu.addAction(QtGui.QAction('Load Device %s'%(i), self))

            # create button
            self.button = MyPushButton(popMenu, i, self)   
            self.button.setText("Load Device %s" %(i))    
            self.button.resize(100, 30)

            # set button context menu policy
            self.button.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)        

            self.Hbox.addWidget(self.button)
	
	self.end_button = QtGui.QPushButton('End',self)
        self.end_button.clicked.connect(lambda : stop())
	
        self.quit_button = QtGui.QPushButton('Quit',self)
        self.quit_button.clicked.connect(lambda : quit(self))
	self.Hbox.addWidget(self.end_button)
        self.Hbox.addWidget(self.quit_button)
        VmasterLayout.addLayout(self.Hbox)

    def buttonXclicked(self, buttonID):
        if buttonID == 0: 
            #do something , call some method ..
            print "button with ID ", buttonID, " is clicked"
        if buttonID == 2: 
            #do something , call some method ..
            print "button with ID ", buttonID, " is clicked"
        if buttonID == 3: 
            #do something , call some method ..
            print "button with ID ", buttonID, " is clicked"
	os.system('rm -rf *.clb;./ocl-example-facedetect ./haarcascade_frontalface_alt.xml  /home/rajaram/Desktop/face-detect-angelina/img.csv ' + str(buttonID) + '&')


def main():
    app = QtGui.QApplication(sys.argv)
    form = MainForm()
    form.show()
    app.exec_()

if __name__ == '__main__':
    main()
