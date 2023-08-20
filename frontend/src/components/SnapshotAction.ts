import {
  CustomVisibilityState,
  FileAction,
  FileActionButton,
  FileActionEffect,
  FileSelectionTransform,
  defineFileAction,
  FileData,
  thunkRequestFileAction,
  ChonkyActions,
  FileHelper,
  selectParentFolder,
  selectCurrentFolder,
  selectFileData,
  selectSelectedFiles,
} from "chonky";

export interface SnapshotActionPayload {
  sourceBranch?: string;
  targetBranch?: string;
}

export const SnapshotAction = defineFileAction({
  id: "snap",
  fileFilter: (file: FileData | null) => {
    return file != null && file.id.split("/").length == 2;
  },
  __payloadType: {} as SnapshotActionPayload,
});

export const SnapToAction = defineFileAction(
  {
    id: "snap_to",
    fileFilter: (file: FileData | null) => {
      console.log(file?.id);
      return file?.id.split("/").length == 2;
    },
    button: {
      name: "Snap to...",
      contextMenu: true,
    },
  } as const,
  ({ reduxDispatch, getReduxState }) => {
    const folder = selectSelectedFiles(getReduxState());
    reduxDispatch(
      thunkRequestFileAction(SnapshotAction, {
        sourceBranch: folder[0].name,
      })
    );
  }
);
