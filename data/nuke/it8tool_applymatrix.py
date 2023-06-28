import nuke
from PySide2.QtWidgets import QApplication

def set_matrix_from_clipboard():

    app = QApplication.instance()
    clipboard = app.clipboard().text()
    matrix_values = clipboard.strip().split()

    if len(matrix_values) != 9:
        nuke.message("Invalid matrix values.")
        return

    matrix = [float(value) for value in matrix_values]
    selected_node = nuke.selectedNode()

    if selected_node.Class() != "ColorMatrix":
        nuke.message("Please select a ColorMatrix node.")
        return

    selected_node["matrix"].setValue(matrix)
    print("Matrix value: " + clipboard)

set_matrix_from_clipboard()