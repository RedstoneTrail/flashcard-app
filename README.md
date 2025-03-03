flashcard-app

its a flashcard app.

h and l to navigate between cards
j and k to manage confidence ratings
space to show the definition
r to go to a ~random card (lower confidence rated cards will appear more frequently)
/ to search (wip)

flashcards are in a database passed as a command line parameter:
```
./flashcard-app flashcards.db
```

the database must match this scheme:
```
flashcard
    flashcardid
    term
    definition
    confidence

flashcardset
    setid
    subject
    topic

cardinset
    flashcardid
    setid
```

compile flashcards to a database with flashcardc:
```
./flashcardc flashcards.toml
```

flashcards.toml will have the following format:
```toml
[[cards]]
term = "Term"
definition = "Definition"
sets = [ "Subject1/Topic1", "Subject2/Topic1" ]
\# optional
confidence = 3
```

examples will appear at some point for each file and my flashcards will be made on a new repo (also at some point, ill write it here)
