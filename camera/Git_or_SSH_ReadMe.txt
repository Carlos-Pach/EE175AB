
============================================================================================================================================
============================================================================================================================================
SSH:

error?:
IF ssh -vvv looks something like this:
	ssh -vvv myusername@myschool.edu
	OpenSSH_for_Windows_7.7p1, LibreSSL 2.6.5
	debug3: Failed to open file:C:/Users/copyp/.ssh/config error:2
	debug3: Failed to open file:C:/ProgramData/ssh/ssh_config error:2
	debug2: resolving "myschool.edu" port 22
	debug2: ssh_connect_direct: needpriv 0
	debug1: Connecting to myschool.edu [12.12.12.12] port 22.
	debug3: finish_connect - ERROR: async io completed with error: 10060, io:0000022866901A50
	debug1: connect to address 12.12.12.12 port 22: Connection timed out
	ssh: connect to host myschool.edu port 22: Connection timed out

CHECK you are connected to the right network that you first used to connect to the Pi (or etc.)
(Ex. I am now connected to Network1, but actually I used NetworkHome to initially connect, so this
error happened, and I thought it was a firewall/permissions/router reset issue:
all I needed to do was to change back to the correct wifi and everything is fine).


ALSO.
	make sure the ip address you connect to (ex. Pi) is the same and doesn't reset after
	for ex. the router resets. For example, this happened to my pi, so I set it to a 
	static IP address which solved my problem.


For VSC to load xml-forwarding (basically show other active windows like webcam, picamera, etc.)
You need to enable the remote-xml extensions (both), 
checkmark both for forwarding, and 
make sure you enable ssh-key, with no passphrase.


============================================================================================================================================
============================================================================================================================================
============================================================================================================================================
GITHUB / GIT :

https://devconnected.com/how-to-push-git-branch-to-remote/
LINKS TO LOOK AT :
https://stackoverflow.com/questions/36132956/how-to-connect-local-folder-to-git-repository-and-start-making-changes-on-branch
https://stackoverflow.com/questions/15285332/is-it-possible-to-track-only-a-folder-in-a-git-branch
https://stackoverflow.com/questions/1279533/is-there-a-way-to-tell-git-to-only-include-certain-files-instead-of-ignoring-cer
https://stackoverflow.com/questions/12799855/configure-git-to-track-only-one-file-extension
https://stackoverflow.com/questions/4917472/add-new-local-directory-to-a-git-remote-branch
https://superuser.com/questions/1412078/bring-a-local-folder-to-remote-git-repo
https://stackoverflow.com/questions/36132956/how-to-connect-local-folder-to-git-repository-and-start-making-changes-on-branch

git init (initialize a repo in local)

git status 
(and git status -uno will tell you whether the branch you are tracking is ahead, behind or has diverged. 
If it says nothing, the local and remote are the same.)


HOW TO ADD FILE TO REPO : https://docs.gitlab.com/ee/gitlab-basics/add-file.html    )
git pull origin remotebranch( to get the same files locally at your laptop as the git repo main online)

git fetch origin (fetches all the remote branches) or git fetch --all
(look at all branches  git branch -v -a   )

git checkout branchname  
(Or do:) 
git checkout -b kellyBranch origin/kellyBranch 
	git checkout <non-branch>, for example git checkout origin/test results in detached HEAD / unnamed branch,
 	while git checkout test or git checkout -b test origin/test results in local branch test (with remote-tracking branch origin/test as upstream)

git add .  (to add all, or do git add filename for individual)
git commit -m "words"
	(to undo from commit:)
	git reset       
git push -u origin edit_branch_01   #-u option sets up an upstream tracking branch


-------------------------------------------------------------
---------------------------------------------------------------------------
GIT PUSH ERRORS?
ex.
	PS C:\tensorflow1\EE175AB> git push -u origin kellyBranch
	Enumerating objects: 39, done.
	Counting objects: 100% (39/39), done.
	Delta compression using up to 4 threads
	Compressing objects: 100% (37/37), done.
->	error: RPC failed; curl 92 HTTP/2 stream 0 was not closed cleanly: CANCEL (err 8)
->	fatal: the remote end hung up unexpectedly
	Writing objects: 100% (38/38), 430.53 MiB | 4.53 MiB/s, done.
	Total 38 (delta 14), reused 0 (delta 0), pack-reused 0
	fatal: the remote end hung up unexpectedly

	(error 8, which is not really talked about in google searches, so I found alternate solutions like:

	git config --global http.version HTTP/1.1
	(then git push o branchname again, however gave me a new error that is luckily more documented:
	git push -u origin kellyBranch
	Enumerating objects: 39, done.
	Counting objects: 100% (39/39), done.
	Delta compression using up to 4 threads
	Compressing objects: 100% (37/37), done.
->	error: RPC failed; curl 18 transfer closed with outstanding read data remaining
	fatal: the remote end hung up unexpectedly
	Writing objects: 100% (38/38), 430.53 MiB | 4.30 MiB/s, done.
	Total 38 (delta 14), reused 0 (delta 0), pack-reused 0
	fatal: the remote end hung up unexpectedly	
	Everything up-to-date
	)
	SOLUTION: #bc of large file size(to increase 'http.postBuffer' so you can send more data across to github, increase by:)
	git config --global http.postBuffer 524288000  


	then i went back to git config --global http.version HTTP/1.1   and then git push but I'm not sure if we need to do it
	git config --global http.version HTTP/2  #to reset back if needed
