[CmdletBinding()]
param([Parameter(Mandatory)] [ValidateSet('uninstall','update','rollback')] [string] $Action)
throw "$Action is SCAFFOLDED / NOT VALIDATED: channel-safe installed ownership is not implemented. Read docs/INSTALLATION.md. No files were changed."
