Attribute VB_Name = "Module2"
' Set up columns to maintain Otto plans
' Otto version 2.0.2

Sub BootstrapOttoProject_2_0_2()
' Set up columns to maintain Otto plans
    SelectTaskColumn Column:="Resource Names"
    ColumnDelete
    SelectTaskColumn Column:="Finish"
    ColumnDelete
    SelectTaskColumn Column:="Start"
    ColumnDelete
    SelectTaskColumn Column:="Task Mode"
    ColumnDelete
    SelectTaskColumn Column:="Indicators"
    ColumnDelete

    TableEditEx Name:="&Entry", TaskTable:=True, FieldName:="Name", NewFieldName:="Name", Title:="jobname", ShowInMenu:=True, ColumnPosition:=0
    TableApply Name:="&Entry"

    TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="Text1", ShowInMenu:=True, ColumnPosition:=3
    TableApply Name:="&Entry"
    TableEditEx Name:="&Entry", TaskTable:=True, FieldName:="Text1", Title:="command", ShowInMenu:=True
    TableApply Name:="&Entry"
    
    TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="Text3", ShowInMenu:=True, ColumnPosition:=4
    TableApply Name:="&Entry"
    TableEditEx Name:="&Entry", TaskTable:=True, FieldName:="Text3", Title:="environment", ShowInMenu:=True
    TableApply Name:="&Entry"
    
    TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="Text7", ShowInMenu:=True, ColumnPosition:=5
    TableApply Name:="&Entry"
    TableEditEx Name:="&Entry", TaskTable:=True, FieldName:="Text7", Title:="loop", ShowInMenu:=True
    TableApply Name:="&Entry"
  
    TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="Flag1", ShowInMenu:=True, ColumnPosition:=6
    TableApply Name:="&Entry"
    TableEditEx Name:="&Entry", TaskTable:=True, FieldName:="Flag1", Title:="auto_hold", ShowInMenu:=True
    TableApply Name:="&Entry"
    
    TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="Flag2", ShowInMenu:=True, ColumnPosition:=7
    TableApply Name:="&Entry"
    TableEditEx Name:="&Entry", TaskTable:=True, FieldName:="Flag2", Title:="date_conditions", ShowInMenu:=True
    TableApply Name:="&Entry"
    
    TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="Text4", ShowInMenu:=True, ColumnPosition:=8
    TableApply Name:="&Entry"
    TableEditEx Name:="&Entry", TaskTable:=True, FieldName:="Text4", Title:="days_of_week", ShowInMenu:=True
    TableApply Name:="&Entry"
    
    TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="Text5", ShowInMenu:=True, ColumnPosition:=9
    TableApply Name:="&Entry"
    TableEditEx Name:="&Entry", TaskTable:=True, FieldName:="Text5", Title:="start_mins", ShowInMenu:=True
    TableApply Name:="&Entry"
    
    TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="Text6", ShowInMenu:=True, ColumnPosition:=10
    TableApply Name:="&Entry"
    TableEditEx Name:="&Entry", TaskTable:=True, FieldName:="Text6", Title:="start_times", ShowInMenu:=True
    TableApply Name:="&Entry"
    
    TableEditEx Name:="&Entry", TaskTable:=True, NewFieldName:="Text2", ShowInMenu:=True, ColumnPosition:=11
    TableApply Name:="&Entry"
    TableEditEx Name:="&Entry", TaskTable:=True, FieldName:="Text2", Title:="description", ShowInMenu:=True
    TableApply Name:="&Entry"
    
    InsertSummaryTask
    SetTaskField Field:="Name", Value:="CNV_UNLOAD"
    SetTaskField Field:="Text2", Value:="Control data unloads."
    SelectTaskField Row:=1, Column:="Name"
    SetTaskField Field:="Name", Value:="CNV_UNLOAD_NOTIFY"
    SetTaskField Field:="Text1", Value:="cnv_ottojob.sh cnv_send_msg.sh ALL Data unloads have started."
    SetTaskField Field:="Text2", Value:="Notify team of status."
    SetTaskField Field:="Duration", Value:="1 min"
    
    SelectTaskField Row:=1, Column:="Name"
    InsertSummaryTask
    SetTaskField Field:="Name", Value:="CNV_CONVERT"
    OutlineOutdent
    SelectTaskField Row:=0, Column:="Predecessors"
    SetTaskField Field:="Predecessors", Value:="1"
    SetTaskField Field:="Text2", Value:="Control data conversions."
    SelectTaskField Row:=1, Column:="Name"
    SetTaskField Field:="Name", Value:="CNV_CONVERT_NOTIFY"
    SetTaskField Field:="Text1", Value:="cnv_ottojob.sh cnv_send_msg.sh ALL Data conversion has started."
    SetTaskField Field:="Text2", Value:="Notify team of status."
    SetTaskField Field:="Duration", Value:="1 min"
    
    SelectTaskField Row:=1, Column:="Name"
    InsertSummaryTask
    SetTaskField Field:="Name", Value:="CNV_LOAD"
    OutlineOutdent
    SelectTaskField Row:=0, Column:="Predecessors"
    SetTaskField Field:="Predecessors", Value:="3"
    SetTaskField Field:="Text2", Value:="Control data loads."
    SelectTaskField Row:=1, Column:="Name"
    SetTaskField Field:="Name", Value:="CNV_LOAD_NOTIFY"
    SetTaskField Field:="Text1", Value:="cnv_ottojob.sh cnv_send_msg.sh ALL Data loads have started."
    SetTaskField Field:="Text2", Value:="Notify team of status."
    SetTaskField Field:="Duration", Value:="1 min"
    
    TimescaleEdit MajorUnits:=5, MinorUnits:=6, MajorLabel:=30, MinorLabel:=34, MinorTicks:=True, Separator:=True, MajorUseFY:=True, MinorUseFY:=True, TierCount:=2
    ProjectSummaryInfo Calendar:="24 Hours"
  
    ColumnBestFit Column:=1
    ColumnBestFit Column:=2
    ColumnBestFit Column:=3
    ColumnBestFit Column:=4
    ColumnBestFit Column:=5
    ColumnBestFit Column:=6
    ColumnBestFit Column:=7
    ColumnBestFit Column:=8
    ColumnBestFit Column:=9
    ColumnBestFit Column:=10
    ColumnBestFit Column:=11
    ColumnBestFit Column:=12
    ColumnBestFit Column:=13
    
    BarRounding On:=False
    TimelineViewToggle
    ZoomTimescale Entire:=True
End Sub


