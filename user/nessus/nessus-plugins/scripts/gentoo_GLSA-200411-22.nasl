# This script was automatically generated from 
#  http://www.gentoo.org/security/en/glsa/glsa-200411-22.xml
# It is released under the Nessus Script Licence.
# The messages are release under the Creative Commons - Attribution /
# Share Alike license. See http://creativecommons.org/licenses/by-sa/2.0/
#
# Avisory is copyright 2001-2005 Gentoo Foundation, Inc.
# GLSA2nasl Convertor is copyright 2004 Michel Arboi <mikhail@nessus.org>

if (! defined_func('bn_random')) exit(0);

if (description)
{
 script_id(15723);
 script_version("$Revision: 1.3 $");
 script_xref(name: "GLSA", value: "200411-22");
 script_cve_id("CVE-2004-0972");

 desc = 'The remote host is affected by the vulnerability described in GLSA-200411-22
(Davfs2, lvm-user: Insecure tempfile handling)


    Florian Schilhabel from the Gentoo Linux Security Audit Team found
    that Davfs2 insecurely created .pid files in /tmp. Furthermore, Trustix
    Secure Linux found that the lvmcreate_initrd script, included in the
    lvm-user Gentoo package, also creates temporary files in
    world-writeable directories with predictable names.
  
Impact

    A local attacker could create symbolic links in the temporary
    files directory, pointing to a valid file somewhere on the filesystem.
    When Davfs2 or lvmcreate_initrd is called, this would result in the
    file being overwritten with the rights of the user running the
    software, which could be the root user.
  
Workaround

    There is no known workaround at this time.
  
References:
    http://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2004-0972


Solution: 
    All Davfs2 users should upgrade to the latest version:
    # emerge --sync
    # emerge --ask --oneshot --verbose ">=net-fs/davfs2-0.2.2-r1"
    All lvm-user users should upgrade to the latest version:
    # emerge --sync
    # emerge --ask --oneshot --verbose ">=sys-fs/lvm-user-1.0.7-r2"
  

Risk factor : Medium
';
 script_description(english: desc);
 script_copyright(english: "(C) 2005 Michel Arboi <mikhail@nessus.org>");
 script_name(english: "[GLSA-200411-22] Davfs2, lvm-user: Insecure tempfile handling");
 script_category(ACT_GATHER_INFO);
 script_family(english: "Gentoo Local Security Checks");
 script_dependencies("ssh_get_info.nasl");
 script_require_keys('Host/Gentoo/qpkg-list');
 script_summary(english: 'Davfs2, lvm-user: Insecure tempfile handling');
 exit(0);
}

include('qpkg.inc');
if (qpkg_check(package: "sys-fs/lvm-user", unaffected: make_list("ge 1.0.7-r2"), vulnerable: make_list("lt 1.0.7-r2")
)) { security_warning(0); exit(0); }
if (qpkg_check(package: "net-fs/davfs2", unaffected: make_list("ge 0.2.2-r1"), vulnerable: make_list("lt 0.2.2-r1")
)) { security_warning(0); exit(0); }
