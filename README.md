Dataset Information
HCV data: https://archive.ics.uci.edu/dataset/571/hcv+data
# Instances
615 
# Features
12
# What do the instances in this dataset represent?
Instances are patients

# Additional Information
The target attribute for classification is Category (blood donors vs. Hepatitis C, including its progress: 'just' Hepatitis C, Fibrosis, Cirrhosis).

Variable Name	Role	Type	Demographic	Description Units	Missing Values
ID	ID	Integer		Patient ID		no
Age	Feature	Integer	Age		years	no
Sex	Feature	Binary	Sex			no
ALB	Feature	Continuous				yes
ALP	Feature	Continuous				yes
AST	Feature	Continuous				yes
BIL	Feature	Continuous				no
CHE	Feature	Continuous				no
CHOL	Feature	Continuous				yes
CREA	Feature	Continuous				no
CGT	Feature	Continuous				no
PROT	Feature	Continuous				yes
Category	Target	Categorical		values: '0=Blood Donor', '0s=suspect Blood Donor', '1=Hepatitis', '2=Fibrosis', '3=Cirrhosis'		no
ALT	Feature	Continuous				no
