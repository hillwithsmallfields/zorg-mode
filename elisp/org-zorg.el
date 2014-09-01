;;;; Convert org files to zorg files

;;; A .zorg file is a bit like a .org file, but the repeated
;;; characters marking a heading have been replaced by a digit
;;; representing the number of them, keywords will be replaced by a
;;; short indication, with a keywords line in the file indicating the
;;; expansions, and the tags likewise.  The keywords line begins with
;;; an exclamation mark, and has space-separated keywords, with a
;;; vertical bar (with a space on either side of it) marking the
;;; division between not-done and done, and an exclamation mark (with
;;; a space on either side of it) marking the boundary at which
;;; cycling a keyword should go back.  The tags line begins with a
;;; colon, and has a colon before each tag, and none at the end.

(defun org-dates-in-buffer ()
  "Return a list of the dates in the current buffer."
  (save-excursion
    (let ((results nil))
      (goto-char (point-min))
      (while (re-search-forward "<\\([0-9]\\{4\\}-[0-9][0-9]-[0-9][0-9] [A-Z][a-z][a-z]\\( [0-9:]+\\)?\\)>" (point-max) t)
	(let ((date (match-string-no-properties 1)))
	  (unless (member date results)
	    (push date results))))
      (sort results 'string<))))

(defun org-export-to-zorg (org-file zorg-file)
  "Convert ORG-FILE to ZORG-FILE."
  (interactive "fFile to export from:
FFile to export into: ")
  (save-excursion
    (find-file zorg-file)
    (erase-buffer)
    (insert-file-contents org-file)
    (org-mode)
    (let ((file-tags (mapcar 'car (org-get-buffer-tags)))
	  (file-keywords
	   (apply 'append
		  (cons '("!")
			(mapcar (lambda (kl)
				  (org-remove-keyword-keys (cdr kl)))
				org-todo-keywords))))
	  (file-dates (org-dates-in-buffer)))
      (goto-char (point-min))
      (delete-non-matching-lines "^[* \t]")
      (goto-char (point-min))
      (while (not (eobp))
	(cond
	 ((looking-at "^\\(\\*+\\)")
	  (let ((line-tags (save-match-data (org-get-tags))))
	    (message "file-tags %S; line-tags %S" file-tags line-tags)

	    ;; turn * sequences into numbers
	    ;; todo: check they're not too large
	    (replace-match (int-to-string (- (match-end 1) (match-beginning 1))) nil t nil 1)

	    ;; convert keyword to index in file-level list of keywords
	    (beginning-of-line 1) (forward-char 2)
	    (when (looking-at org-todo-regexp)
	      (let* ((keyword  (match-string-no-properties 1))
		     (pos (position keyword (cdr file-keywords) :test 'string=)))
		(if pos
		    (replace-match (format "!%d" pos))
		  ;; todo: fix picking up the file keyword list properly
		(message "Warning: %S not in keywords!" keyword))))

	    ;; convert tags into list of indices in file-level list of keywords
	    (when line-tags
	      (insert " :"
		      (mapconcat (lambda (tag)
				   (int-to-string (position tag file-tags :test 'string=)))
				 line-tags
				 ":")
		      " ")
	      ;; todo: remove original tags
	      
	      )
	    ))
	 ((looking-at "^\\(\\s-+\\)")
	  (message "continuation line")
	  ;; start other lines with a single space
	  (replace-match " " nil t nil 1))
	 (t
	  (insert " "))
	 )
	(beginning-of-line 2))
      (goto-char (point-min))
      (fundamental-mode)
      (insert (mapconcat 'identity
			 file-keywords
			 " ")
	      "\n")
      (insert ":"
	      (mapconcat 'identity file-tags ":")
	      "\n")
      (when file-dates
	(insert "@"
		(mapconcat 'identity file-dates ",")
		"\n"))
      ;; todo: output list of dates, in some compact form
      )
    (basic-save-buffer)))

(defun org-export-directory-to-zorg (org-dir zorg-dir)
  "Export all .org files in ORG-DIR to ZORG-DIR."
  (interactive "DExport org files from: 
DExport to: ")
  (dolist (org-file (directory-files org-dir t "\\.org$" t))
    (org-export-to-zorg org-file (expand-file-name (file-name-nondirectory org-file)
						   zorg-dir))))

;;;; org-zorg.el ends here
