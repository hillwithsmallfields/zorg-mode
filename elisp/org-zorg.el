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
				org-todo-keywords)))))
      (goto-char (point-min))
      (delete-non-matching-lines "^\\*")
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
	      (replace-match (format "!%d" (position (match-string-no-properties 1) file-keywords :test 'string=))))

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
	      "\n"))
    (basic-save-buffer))
  )

;;;; org-zorg.el ends here
