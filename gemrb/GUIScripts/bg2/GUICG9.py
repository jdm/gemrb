#character generation, proficiencies (GUICG9)
import GemRB

SkillWindow = 0
TextAreaControl = 0
DoneButton = 0
SkillTable = 0
TopIndex = 0
RowCount = 0
PointsLeft = 0
ProfColumn = 0

def RedrawSkills():
	SumLabel = GemRB.GetControl(SkillWindow, 0x10000009)
	GemRB.SetText(SkillWindow, SumLabel, str(PointsLeft) )  #points to distribute

	for i in range(0,8):
		Pos=TopIndex+i
		MaxProf = GemRB.GetTableValue(SkillTable, Pos+8, ProfColumn) #we add the bg1 skill count offset
		print "MaxProf:",MaxProf," row:",Pos+8," in column:",ProfColumn
		Label=GemRB.GetControl(SkillWindow, i+69)
		Button1=GemRB.GetControl(SkillWindow, i*2+11)
		Button2=GemRB.GetControl(SkillWindow, i*2+12)
		if MaxProf == 0:
			GemRB.SetButtonState(SkillWindow, Label, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonState(SkillWindow, Button1, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonState(SkillWindow, Button2, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags(SkillWindow, Button1, IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			GemRB.SetButtonFlags(SkillWindow, Button2, IE_GUI_BUTTON_NO_IMAGE,OP_OR)
		else:
			GemRB.SetButtonState(SkillWindow, Label, IE_GUI_BUTTON_ENABLED)
			GemRB.SetButtonState(SkillWindow, Button1, IE_GUI_BUTTON_ENABLED)
			GemRB.SetButtonState(SkillWindow, Button2, IE_GUI_BUTTON_ENABLED)
			GemRB.SetButtonFlags(SkillWindow, Button1, IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			GemRB.SetButtonFlags(SkillWindow, Button2, IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
		
		SkillName = GemRB.GetTableValue(SkillTable, Pos+8, 1) #we add the bg1 skill count offset
		Label=GemRB.GetControl(SkillWindow, 0x10000001+i)
		GemRB.SetText(SkillWindow, Label, SkillName)

		ActPoint = GemRB.GetVar("Proficiency "+str(Pos) )
		for j in range(0,5):  #5 is maximum distributable
			Star=GemRB.GetControl(SkillWindow, i*5+j+27)
			if ActPoint >j:
				GemRB.SetButtonFlags(SkillWindow, Star, IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			else:
				GemRB.SetButtonFlags(SkillWindow, Star, IE_GUI_BUTTON_NO_IMAGE,OP_OR)
	return

def OnLoad():
	global SkillWindow, TextAreaControl, DoneButton, TopIndex
	global SkillTable, PointsLeft, ProfColumn
	
	ClassTable = GemRB.LoadTable("classes")
	Class = GemRB.GetVar("Class")-1
	ClassName = GemRB.GetTableRowName(ClassTable, Class)
	Kit = GemRB.GetVar("Class Kit")
	if Kit == 0:
		KitName = ClassName
		ProfColumn = Class + 3
	else:
		KitList = GemRB.LoadTable("kitlist")
		#rowname is just a number, the kitname is the first data column
		KitName = GemRB.GetTableValue(KitList, Kit, 0)
		#this is the proficiency column number in kitlist
		ProfColumn = GemRB.GetTableValue(KitList, Kit, 5)

	SkillTable = GemRB.LoadTable("profs")
	Level = GemRB.GetVar("Level")-1
	PointsLeft = GemRB.GetTableValue(SkillTable, ClassName, "FIRST_LEVEL")
	if Level>0:
		PointsLeft=PointsLeft + Level/GemRB.GetTableValue(SkillTable, ClassName, "RATE")

	GemRB.LoadWindowPack("GUICG")
        SkillTable = GemRB.LoadTable("weapprof")
	RowCount = GemRB.GetTableRowCount(SkillTable)-7  #we decrease it with the bg1 skills
	SkillWindow = GemRB.LoadWindow(9)

	for i in range(0,8):
		Button=GemRB.GetControl(SkillWindow, i+69)
		GemRB.SetVarAssoc(SkillWindow, Button, "Skill", i)
		GemRB.SetEvent(SkillWindow, Button, IE_GUI_BUTTON_ON_PRESS, "JustPress")

		Button=GemRB.GetControl(SkillWindow, i*2+11)
		GemRB.SetVarAssoc(SkillWindow, Button, "Skill", i)
		GemRB.SetEvent(SkillWindow, Button, IE_GUI_BUTTON_ON_PRESS, "LeftPress")

		Button=GemRB.GetControl(SkillWindow, i*2+12)
		GemRB.SetVarAssoc(SkillWindow, Button, "Skill", i)
		GemRB.SetEvent(SkillWindow, Button, IE_GUI_BUTTON_ON_PRESS, "RightPress")

		for j in range(0,5):
			Star=GemRB.GetControl(SkillWindow, i*5+j+27)
			GemRB.SetButtonState(SkillWindow, Star, IE_GUI_BUTTON_DISABLED)

	BackButton = GemRB.GetControl(SkillWindow,77)
	GemRB.SetText(SkillWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(SkillWindow,0)
	GemRB.SetText(SkillWindow,DoneButton,11973)

	TextAreaControl = GemRB.GetControl(SkillWindow, 68)
	GemRB.SetText(SkillWindow,TextAreaControl,9588)

	ScrollBarControl = GemRB.GetControl(SkillWindow, 78)
	GemRB.SetVarAssoc(SkillWindow, ScrollBarControl, "TopIndex", RowCount)
	GemRB.SetEvent(SkillWindow, ScrollBarControl, IE_GUI_SCROLLBAR_ON_CHANGE, "ScrollBarPress")

	GemRB.SetEvent(SkillWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(SkillWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetButtonState(SkillWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	TopIndex = 0
	RedrawSkills()
	GemRB.SetVisible(SkillWindow,1)
	return

def ScrollBarPress():
	TopIndex = GemRB.GetVar("TopIndex")
	RedrawSkills()
	return

def JustPress():
	Pos = GemRB.GetVar("Skill")+TopIndex
	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable, Pos+8, 2) )
	return
	
def RightPress():
	Pos = GemRB.GetVar("Skill")+TopIndex
	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable, Pos+8, 2) )
	ActPoint = GemRB.GetVar("Skill "+str(Pos) )
	if ActPoint <= 0:
		return
	GemRB.SetVar("Skill "+str(Pos),ActPoint-1)
	PointsLeft = PointsLeft + 1
	RedrawSkills()
	return

def LeftPress():
	Pos = GemRB.GetVar("Skill")+TopIndex
	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable, Pos+8, 2) )
	if PointsLeft == 0:
		return
	MaxProf = GemRB.GetTableValue(SkillTable, Pos+8, ProfColumn) #we add the bg1 skill count offset
	if MaxProf>5:
		MaxProf = 5

	ActPoint = GemRB.GetVar("Skill "+str(Pos) )
	if ActPoint >= MaxProf:
		return
	GemRB.SetVar("Skill "+str(Pos),ActPoint+1)
	PointsLeft = PointsLeft - 1
	RedrawSkills()
	return

def BackPress():
	GemRB.UnloadWindow(SkillWindow)
	GemRB.SetNextScript("CharGen6")
	#scrap skills
	return

def NextPress():
        GemRB.UnloadWindow(SkillWindow)
	GemRB.SetNextScript("CharGen7") #appearance
	return
