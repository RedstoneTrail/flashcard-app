flashcard-app

its a flashcard app.

h and l to navigate between cards
j and k to manage confidence ratings
space to show the definition
r to go to a ~random card (lower confidence rated cards will appear more frequently)
/ to search (wip)

flashcards are in a database passed as a command line parameter:
'''
./flashcard-app flashcards.db
'''

the database must match this scheme:
'''
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
'''

compile flashcards to a database with flashcardc:
'''
./flashcardc -f flashcards.txt -s sets.txt -r relations.txt
'''

flashcards.txt has the following format:
    the line number is the cardid + 1 (database is 0-indexed, line numbers are 1-indexed)
    the term and definition for a card are on the same line, term first, separated by 3 '$' symbols with spaces either side ( $$$ )

sets.txt has the following format:
    the line number is the setid + 1 (database is 0-indexed, line numbers are 1-indexed)
    the subject and topic for a card are on the same line, subject first, separated by 3 '$' symbols with spaces either side ( $$$ )
    for a subject or topic-agnostic card, you can leave the section blank

relations.txt has the following format:
    the flashcardid and setid for a card are on the same line, flashcardid first, separated by 3 '$' symbols with spaces either side ( $$$ )

examples will appear at some point for each file and my flashcards will be made on a new repo (also at some point, ill write it here)
