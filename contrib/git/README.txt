Github doesn't send good diff emails, so we do it ourselves.

On mtt.open-mpi.org, we have a git clone from github.  We then also
have a local naked git clone that has a post-receive hook that sends
diff emails.

So via cron, we git pull from github, and then git push to the local
naked git repo, which then triggers sending the emails.

It's not perfect, but it seems to be good enough.
