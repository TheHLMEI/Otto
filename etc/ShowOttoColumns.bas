' Set up columns to maintain Otto plans
' Otto version 2.1.0

Sub ShowOttoColumns_2_1_0()


   Dim i, iColumn As Integer
   Dim c, ccommand, cenvironment, cloop, cautohold, cdate_conditions, cdays_of_week, cstart_mins, cstart_times, cdescription As Long

   ccommand = -1
   cenvironment = -1
   cloop = -1
   cauto_hold = -1
   cauto_noexec = -1
   cdate_conditions = -1
   cdays_of_week = -1
   cstart_mins = -1
   cstart_times = -1
   cdescription = -1

   ViewApplyEx Name:="Timeline", ApplyTo:=0        'Switch to single Timeline View to export
   'TimelineViewToggle                              'add the Timeline View window
   ViewApply Name:="Gantt chart"

   RemoveColumn ("Resource Names")
   RemoveColumn ("Finish")
   RemoveColumn ("Start")
   RemoveColumn ("Task Mode")
   RemoveColumn ("Indicators")

   RemoveColumn ("command")
   RemoveColumn ("environment")
   RemoveColumn ("loop")
   RemoveColumn ("auto_hold")
   RemoveColumn ("auto_noexec")
   RemoveColumn ("date_conditions")
   RemoveColumn ("days_of_week")
   RemoveColumn ("start_mins")
   RemoveColumn ("start_times")
   RemoveColumn ("description")



   TableEditEx Name:="&Entry", TaskTable:=True, FieldName:="Name", NewFieldName:="Name", Title:="Otto Job Name", ShowInMenu:=True, ColumnPosition:=0

   For i = 1 To 30
c = FieldNameToFieldConstant("Text" & i, pjTask) ' get constant of custom field by name
myName = CustomFieldGetName(c)

   If (myName = "command" Or myName = "Command") Then
   ccommand = c
   End If

   If (myName = "description" Or myName = "Description") Then
   cdescription = c
   End If

   If (myName = "environment" Or myName = "Environment") Then
   cenvironment = c
   End If

   If (myName = "days_of_week") Then
   cdays_of_week = c
   End If

   If (myName = "start_mins") Then
   cstart_mins = c
   End If

   If (myName = "start_times") Then
   cstart_times = c
   End If

   If (myName = "loop" Or myName = "Loop") Then
   cloop = c
   End If
   Next

   For i = 1 To 20
c = FieldNameToFieldConstant("Flag" & i, pjTask) ' get constant of custom field by name
myName = CustomFieldGetName(c)

   If (myName = "auto_hold") Then
   cauto_hold = c
   End If

   If (myName = "auto_noexec") Then
   cauto_noexec = c
   End If

   If (myName = "date_conditions") Then
   cdate_conditions = c
   End If
   Next

   iColumn = 2
   If (ccommand > 0) Then
   iColumn = iColumn + 1
   TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="command", ShowInMenu:=True, ColumnPosition:=iColumn
   End If

   If (cenvironment > 0) Then
   iColumn = iColumn + 1
   TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="environment", ShowInMenu:=True, ColumnPosition:=iColumn
   End If

   If (cloop > 0) Then
   iColumn = iColumn + 1
   TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="loop", ShowInMenu:=True, ColumnPosition:=iColumn
   End If

   If (cauto_hold > 0) Then
   iColumn = iColumn + 1
   TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="auto_hold", ShowInMenu:=True, ColumnPosition:=iColumn
   End If

   If (cauto_noexec > 0) Then
   iColumn = iColumn + 1
   TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="auto_noexec", ShowInMenu:=True, ColumnPosition:=iColumn
   End If

   If (cdate_conditions > 0) Then
   iColumn = iColumn + 1
   TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="date_conditions", ShowInMenu:=True, ColumnPosition:=iColumn
   End If

   If (cdays_of_week > 0) Then
   iColumn = iColumn + 1
   TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="days_of_week", ShowInMenu:=True, ColumnPosition:=iColumn
   End If

   If (cstart_mins > 0) Then
   iColumn = iColumn + 1
   TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="start_mins", ShowInMenu:=True, ColumnPosition:=iColumn
   End If

   If (cstart_times > 0) Then
   iColumn = iColumn + 1
   TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="start_times", ShowInMenu:=True, ColumnPosition:=iColumn
   End If

   If (cdescription > 0) Then
   iColumn = iColumn + 1
   TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="description", ShowInMenu:=True, ColumnPosition:=iColumn
   End If

   TableApply Name:="&Entry"

   iColumn = iColumn + 2
   For i = 1 To iColumn
   ColumnBestFit Column:=i
   Next

   BarRounding On:=False
   'TimelineViewToggle
   ZoomTimescale Entire:=True

   For Each tsk In ActiveProject.Tasks
   If tsk.OutlineLevel > 1 Then
   numsubtasks = numsubtasks + 1
   End If
   Next tsk

   If (numsubtasks > 0) Then
   OutlineHideSubTasks
   OutlineShowSubTasks
   End If

   End Sub

Public Function RemoveColumn(Name As String)
   On Error GoTo LeaveRemoveColumn
   SelectTaskColumn Column:=Name ' Will error out if the column is hidden
   ColumnDelete
   LeaveRemoveColumn:
   End Function


