# Our custom DT Binding - Readme

Filename: `lddia,pushbtn.yaml`

Step 1.   
One-time: setup the env correctly with recent ver of dtschema

`python3 -m venv ~/venvs/dtschema

~/venvs/dtschema/bin/pip install --upgrade pip

~/venvs/dtschema/bin/pip install dtschema==2026.9 yamllint`

Step 2.   
Always source it first
source ~/venvs/dtschema/bin/activate

Step 3.   
Validate the binding:
make dt_binding_check DT_SCHEMA_FILES=input/lddia,pushbtn.yaml
