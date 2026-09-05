"""Failure-path tests for owned build artifacts; no actual Trash/installed file access."""
import importlib.util,json,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
spec=importlib.util.spec_from_file_location('release',Path(__file__).resolve().parents[1]/'common/release.py')
release=importlib.util.module_from_spec(spec);spec.loader.exec_module(release)
class ReleaseSafety(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.root=Path(self.temp.name);self.key='windows-x64';self.base=self.root/'out'/self.key;self.base.mkdir(parents=True)
  self.addCleanup(patch.stopall);patch.object(release,'ROOT',self.root).start();patch.object(release,'layout',return_value={'product':'AIFRED 4','channel':'alpha','platforms':{self.key:{'plugin':'native/Aifred.vst3'}}}).start()
 def artifact(self,name,content='new'):
  folder=self.base/name;folder.mkdir();(folder/'.aifred-stage.json').write_text(json.dumps({'product':'AIFRED 4','channel':'alpha'}));(folder/'payload').write_text(content);return folder
 def test_candidate_validation_failure_preserves_current(self):
  current=self.artifact('current','old');self.artifact('stage');patch.object(release,'verify',side_effect=ValueError('bad hash')).start()
  with self.assertRaises(ValueError):release.promote(self.key)
  self.assertEqual((current/'payload').read_text(),'old');self.assertFalse((self.base/'previous').exists())
 def test_post_rename_failure_restores_previous(self):
  self.artifact('current','old');self.artifact('stage');patch.object(release,'verify',side_effect=[None,None,ValueError('post-promotion verification')]).start()
  with self.assertRaises(ValueError):release.promote(self.key)
  self.assertEqual((self.base/'current/payload').read_text(),'old');self.assertEqual((self.base/'stage/payload').read_text(),'new')
 def test_success_recycles_only_after_verified_current(self):
  self.artifact('current','old');self.artifact('stage');verified=patch.object(release,'verify').start()
  def recycle(path,parent):
   self.assertEqual(path,self.base/'previous');self.assertEqual(verified.call_count,3);self.assertEqual((self.base/'current/payload').read_text(),'new')
  mocked=patch.object(release,'recycle',side_effect=recycle).start();release.promote(self.key);mocked.assert_called_once()
 def test_recycle_failure_retains_recovery(self):
  self.artifact('current','old');self.artifact('stage');patch.object(release,'verify').start();patch.object(release,'recycle',side_effect=OSError('trash unavailable')).start();release.promote(self.key)
  self.assertEqual((self.base/'current/payload').read_text(),'new');self.assertEqual((self.base/'previous/payload').read_text(),'old')
 def test_unknown_candidate_is_never_recycled(self):
  folder=self.base/'stage';folder.mkdir();(folder/'user-file').write_text('keep');mocked=patch.object(release,'recycle').start()
  with self.assertRaises(FileNotFoundError):release.prepare(self.key)
  mocked.assert_not_called();self.assertTrue((folder/'user-file').exists())
 def test_recovery_blocks_overwrite(self):
  self.artifact('previous','old');self.artifact('stage');patch.object(release,'verify').start()
  with self.assertRaises(ValueError):release.promote(self.key)
  self.assertTrue((self.base/'previous/payload').exists())
 def test_path_escape_and_owner_root_rejected(self):
  for path in [self.root,self.root/'out',self.root/'out/../outside']:
   with self.assertRaises(ValueError):release.checked_path(path,self.root/'out')
 def test_corrupt_hash_fails_before_component_check(self):
  folder=self.artifact('current');(folder/'manifest.json').write_text(json.dumps({'product':'AIFRED 4','channel':'alpha','platform':self.key,'hashes':{}}))
  with self.assertRaisesRegex(ValueError,'hash mismatch'):release.verify(self.key)
if __name__=='__main__':unittest.main()
